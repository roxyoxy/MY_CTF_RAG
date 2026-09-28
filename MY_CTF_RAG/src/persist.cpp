// persist.cpp -- snapshot persistence (7th contract, include/persist.h).
// The snapshot is a local cache: integers are written at native byte
// order and native width is pinned via fixed-width types. It is not an
// exchange format; a snapshot from another machine is rebuilt, not
// converted.

#include "persist.h"

#include "chunker.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr char kMagic[8] = { 'M', 'Y', 'R', 'A', 'G', 'I', 'D', 'X' };
constexpr size_t kHeaderSize = 24;

// FNV-1a 64-bit. Two different scopes share this one algorithm:
// the payload fingerprint stored in the header (file integrity) and
// the content fingerprints stored in the manifest (corpus identity).
constexpr uint64_t kFnvOffsetBasis = 0xcbf29ce484222325ULL;
constexpr uint64_t kFnvPrime = 0x100000001b3ULL;

uint64_t fnv1a64(const char* data, size_t len)
{
    uint64_t hash = kFnvOffsetBasis;
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<unsigned char>(data[i]);
        hash *= kFnvPrime;
    }
    return hash;
}

// ---- append family: the payload is assembled fully in memory ----
// The header fingerprint covers every payload byte, so it can only be
// computed after the payload exists, yet the header sits at the front
// of the file. Assembling in memory also makes the write a single
// sequential stream.

void append_u32(std::string& buf, uint32_t v)
{
    char tmp[4];
    std::memcpy(tmp, &v, sizeof tmp);
    buf.append(tmp, sizeof tmp);
}

void append_u64(std::string& buf, uint64_t v)
{
    char tmp[8];
    std::memcpy(tmp, &v, sizeof tmp);
    buf.append(tmp, sizeof tmp);
}

void append_str(std::string& buf, const std::string& s)
{
    append_u32(buf, static_cast<uint32_t>(s.size()));
    buf.append(s.data(), s.size());
}

// ---- read family: a cursor over a byte range, bounds-checked ----
// These checks should never fire once the header fingerprint has
// passed; they exist because "never throws" implies "never trusts".
// Every read validates the remaining byte count first; a short
// buffer is a parse failure, never an out-of-bounds access.

struct Reader {
    const char* p;
    const char* end;

    bool read_u32(uint32_t& v)
    {
        if (end - p < 4) return false;
        std::memcpy(&v, p, 4);
        p += 4;
        return true;
    }

    bool read_u64(uint64_t& v)
    {
        if (end - p < 8) return false;
        std::memcpy(&v, p, 8);
        p += 8;
        return true;
    }

    bool read_str(std::string& s)
    {
        uint32_t len = 0;
        if (!read_u32(len)) return false;
        // subtraction form: the on-disk length is untrusted and
        // pointer addition with a huge value could wrap
        if (static_cast<uint64_t>(len) > static_cast<uint64_t>(end - p))
            return false;
        s.assign(p, len);
        p += len;
        return true;
    }

    bool read_byte(unsigned char& v)
    {
        if (end - p < 1) return false;
        v = static_cast<unsigned char>(*p);
        p += 1;
        return true;
    }

    bool skip(size_t n)
    {
        if (static_cast<uint64_t>(n) > static_cast<uint64_t>(end - p))
            return false;
        p += n;
        return true;
    }
};

bool read_file(const std::string& path, std::string& buf)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char block[8192];
    // partial reads set failbit at eof; bad() separates real IO errors
    while (in.read(block, sizeof block) || in.gcount() > 0)
        buf.append(block, static_cast<size_t>(in.gcount()));
    return !in.bad();
}

struct ManifestEntry {
    std::string path;
    uint64_t content_fp = 0;

    bool operator==(const ManifestEntry& other) const
    {
        return path == other.path && content_fp == other.content_fp;
    }
};

// {path, content fingerprint} per document, sorted by path. Sorting
// feeds both deterministic serialization (save) and the linear
// pairwise compare in validate.
std::vector<ManifestEntry> build_manifest(const std::vector<Document>& docs)
{
    std::vector<ManifestEntry> m;
    m.reserve(docs.size());
    for (const Document& d : docs) {
        ManifestEntry e;
        e.path = d.path;
        e.content_fp = fnv1a64(d.content.data(), d.content.size());
        m.push_back(e);
    }
    std::sort(m.begin(), m.end(),
        [](const ManifestEntry& a, const ManifestEntry& b) {
            return a.path < b.path;
        });
    return m;
}

}  // namespace

bool save(const std::string& path,
    const std::vector<Document>& docs,
    const std::vector<Chunk>& chunks,
    const InvertedIndex& index)
{
    std::string payload;

    append_u32(payload, static_cast<uint32_t>(CHUNK_SIZE));
    append_u32(payload, static_cast<uint32_t>(CHUNK_OVERLAP));

    append_u32(payload, static_cast<uint32_t>(docs.size()));
    for (const Document& doc : docs) {
        append_u32(payload, static_cast<uint32_t>(doc.id));
        append_str(payload, doc.path);
        append_str(payload, doc.content);
        payload.push_back(doc.deleted ? '\x01' : '\x00');
    }

    append_u32(payload, static_cast<uint32_t>(chunks.size()));
    for (const Chunk& c : chunks) {
        append_u32(payload, static_cast<uint32_t>(c.id));
        append_u32(payload, static_cast<uint32_t>(c.document_id));
        append_u64(payload, static_cast<uint64_t>(c.begin));
        append_u64(payload, static_cast<uint64_t>(c.end));
        // Chunk::text is not stored: restored on load via
        // content.substr(begin, end - begin).
    }

    // Deterministic serialization: unordered_map iteration order varies
    // with insertion history, so terms are written in byte-lexicographic
    // order. Posting lists keep their contract-guaranteed chunk_id
    // ascending order; only the outer order is pinned here.
    using TermEntry =
        std::pair<const std::string*, const std::vector<Posting>*>;
    std::vector<TermEntry> terms;
    terms.reserve(index.postings.size());
    for (const auto& kv : index.postings)
        terms.emplace_back(&kv.first, &kv.second);
    std::sort(terms.begin(), terms.end(),
        [](const TermEntry& a, const TermEntry& b) {
            return *a.first < *b.first;
        });

    append_u32(payload, static_cast<uint32_t>(terms.size()));
    for (const TermEntry& t : terms) {
        append_str(payload, *t.first);
        append_u32(payload, static_cast<uint32_t>(t.second->size()));
        for (const Posting& post : *t.second) {
            append_u32(payload, static_cast<uint32_t>(post.chunk_id));
            append_u32(payload, static_cast<uint32_t>(post.tf));
        }
    }

    append_u32(payload, static_cast<uint32_t>(index.chunk_lengths.size()));
    for (int len : index.chunk_lengths)
        append_u32(payload, static_cast<uint32_t>(len));
    // avg_chunk_length is not stored: recomputed on load.

    const std::vector<ManifestEntry> manifest = build_manifest(docs);
    append_u32(payload, static_cast<uint32_t>(manifest.size()));
    for (const ManifestEntry& e : manifest) {
        append_str(payload, e.path);
        append_u64(payload, e.content_fp);
    }

    const uint64_t fingerprint = fnv1a64(payload.data(), payload.size());

    std::string file;
    file.reserve(kHeaderSize + payload.size());
    file.append(kMagic, sizeof kMagic);
    append_u32(file, FORMAT_VERSION);
    append_u32(file, PIPELINE_VERSION);
    append_u64(file, fingerprint);
    file.append(payload);

    // Atomic replace: write the full bytes to a temp file, close it,
    // then rename over the target so a crash mid-save never destroys
    // the previous snapshot.
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(file.data(), static_cast<std::streamsize>(file.size()));
        out.flush();
        if (!out) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        // two-step fallback for rename implementations that refuse to
        // replace an existing destination
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(tmp, path, ec);
    }
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

SnapshotStatus validate(const std::string& path,
    const std::vector<Document>& docs)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return SnapshotStatus::NOT_FOUND;

    std::string buf;
    if (!read_file(path, buf))
        return SnapshotStatus::IO_ERROR;

    // check 3: magic -- decidable only when the file holds 8 bytes
    if (buf.size() >= 8 && std::memcmp(buf.data(), kMagic, sizeof kMagic) != 0)
        return SnapshotStatus::BAD_MAGIC;
    // check 4: format + payload fingerprint; a header shorter than
    // 24 bytes is truncation and lives here
    if (buf.size() < kHeaderSize)
        return SnapshotStatus::BAD_FORMAT;
    uint32_t fmt = 0;
    uint32_t pipe = 0;
    uint64_t stored_fp = 0;
    std::memcpy(&fmt, buf.data() + 8, 4);
    std::memcpy(&pipe, buf.data() + 12, 4);
    std::memcpy(&stored_fp, buf.data() + 16, 8);
    if (fmt != FORMAT_VERSION)
        return SnapshotStatus::BAD_FORMAT;
    if (fnv1a64(buf.data() + kHeaderSize, buf.size() - kHeaderSize) != stored_fp)
        return SnapshotStatus::BAD_FORMAT;
    // check 5: pipeline
    if (pipe != PIPELINE_VERSION)
        return SnapshotStatus::BAD_PIPELINE;

    Reader r{ buf.data() + kHeaderSize, buf.data() + buf.size() };

    // check 6: chunk params
    uint32_t cs = 0;
    uint32_t co = 0;
    if (!r.read_u32(cs) || !r.read_u32(co))
        return SnapshotStatus::BAD_FORMAT;
    if (cs != static_cast<uint32_t>(CHUNK_SIZE) ||
        co != static_cast<uint32_t>(CHUNK_OVERLAP))
        return SnapshotStatus::BAD_PARAMS;

    // structural walk: read and discard every section in save order;
    // a short read anywhere below means the structure does not parse
    uint32_t n = 0;
    uint32_t u = 0;
    uint64_t w = 0;
    std::string scratch;

    if (!r.read_u32(n)) return SnapshotStatus::BAD_FORMAT;
    for (uint32_t i = 0; i < n; ++i) {  // documents
        if (!r.read_u32(u) || !r.read_str(scratch) ||
            !r.read_str(scratch) || !r.skip(1))
            return SnapshotStatus::BAD_FORMAT;
    }

    if (!r.read_u32(n)) return SnapshotStatus::BAD_FORMAT;
    for (uint32_t i = 0; i < n; ++i) {  // chunks
        if (!r.read_u32(u) || !r.read_u32(u) ||
            !r.read_u64(w) || !r.read_u64(w))
            return SnapshotStatus::BAD_FORMAT;
    }

    if (!r.read_u32(n)) return SnapshotStatus::BAD_FORMAT;  // postings
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t m = 0;
        if (!r.read_str(scratch) || !r.read_u32(m))
            return SnapshotStatus::BAD_FORMAT;
        for (uint32_t j = 0; j < m; ++j) {
            if (!r.read_u32(u) || !r.read_u32(u))
                return SnapshotStatus::BAD_FORMAT;
        }
    }

    if (!r.read_u32(n)) return SnapshotStatus::BAD_FORMAT;  // chunk_lengths
    for (uint32_t i = 0; i < n; ++i) {
        if (!r.read_u32(u)) return SnapshotStatus::BAD_FORMAT;
    }

    if (!r.read_u32(n)) return SnapshotStatus::BAD_FORMAT;  // manifest
    std::vector<ManifestEntry> disk;
    for (uint32_t i = 0; i < n; ++i) {
        ManifestEntry e;
        if (!r.read_str(e.path) || !r.read_u64(e.content_fp))
            return SnapshotStatus::BAD_FORMAT;
        disk.push_back(e);
    }
    if (r.p != r.end)
        return SnapshotStatus::BAD_FORMAT;  // trailing bytes

    // check 7: manifest vs the freshly loaded corpus
    if (build_manifest(docs) != disk)
        return SnapshotStatus::CORPUS_CHANGED;

    return SnapshotStatus::OK;
}

bool load(const std::string& path,
    std::vector<Document>& docs,
    std::vector<Chunk>& chunks,
    InvertedIndex& index)
{
    std::string buf;
    if (!read_file(path, buf))
        return false;

    // header self-check, defense-in-depth for callers that skip
    // validate (contract clause 3)
    if (buf.size() < kHeaderSize) return false;
    if (std::memcmp(buf.data(), kMagic, sizeof kMagic) != 0) return false;
    uint32_t fmt = 0;
    uint32_t pipe = 0;
    uint64_t stored_fp = 0;
    std::memcpy(&fmt, buf.data() + 8, 4);
    std::memcpy(&pipe, buf.data() + 12, 4);
    std::memcpy(&stored_fp, buf.data() + 16, 8);
    if (fmt != FORMAT_VERSION) return false;
    if (fnv1a64(buf.data() + kHeaderSize, buf.size() - kHeaderSize) != stored_fp)
        return false;
    if (pipe != PIPELINE_VERSION) return false;

    Reader r{ buf.data() + kHeaderSize, buf.data() + buf.size() };

    uint32_t cs = 0;
    uint32_t co = 0;
    if (!r.read_u32(cs) || !r.read_u32(co)) return false;
    if (cs != static_cast<uint32_t>(CHUNK_SIZE) ||
        co != static_cast<uint32_t>(CHUNK_OVERLAP))
        return false;

    // Everything parses into local temporaries; the outputs are not
    // touched until the very last step (all-or-nothing). Counts from
    // disk are untrusted, so no reserve() on them.
    std::vector<Document> docs2;
    std::vector<Chunk> chunks2;
    std::unordered_map<std::string, std::vector<Posting>> postings2;
    std::vector<int> lengths2;
    uint32_t n = 0;

    if (!r.read_u32(n)) return false;
    for (uint32_t i = 0; i < n; ++i) {  // documents
        Document d;
        uint32_t id = 0;
        unsigned char del = 0;
        if (!r.read_u32(id)) return false;
        d.id = static_cast<int>(id);
        if (!r.read_str(d.path) || !r.read_str(d.content)) return false;
        if (!r.read_byte(del)) return false;
        d.deleted = (del != 0);
        docs2.push_back(std::move(d));
    }

    if (!r.read_u32(n)) return false;
    for (uint32_t i = 0; i < n; ++i) {  // chunks
        Chunk c;
        uint32_t id = 0;
        uint32_t did = 0;
        uint64_t b = 0;
        uint64_t e = 0;
        if (!r.read_u32(id) || !r.read_u32(did) ||
            !r.read_u64(b) || !r.read_u64(e))
            return false;
        c.id = static_cast<int>(id);
        c.document_id = static_cast<int>(did);
        c.begin = static_cast<size_t>(b);
        c.end = static_cast<size_t>(e);
        chunks2.push_back(std::move(c));
    }

    if (!r.read_u32(n)) return false;
    for (uint32_t i = 0; i < n; ++i) {  // postings
        std::string term;
        uint32_t m = 0;
        if (!r.read_str(term) || !r.read_u32(m)) return false;
        std::vector<Posting> list;
        for (uint32_t j = 0; j < m; ++j) {
            uint32_t cid = 0;
            uint32_t tf = 0;
            if (!r.read_u32(cid) || !r.read_u32(tf)) return false;
            // structural consistency: a posting must reference an
            // existing chunk, search indexes chunks by id directly
            if (cid >= chunks2.size()) return false;
            Posting post;
            post.chunk_id = static_cast<int>(cid);
            post.tf = static_cast<int>(tf);
            list.push_back(post);
        }
        postings2[term] = std::move(list);
    }

    if (!r.read_u32(n)) return false;
    for (uint32_t i = 0; i < n; ++i) {  // chunk_lengths
        uint32_t len = 0;
        if (!r.read_u32(len)) return false;
        lengths2.push_back(static_cast<int>(len));
    }

    // manifest: parsed for structure only; validate() owns the corpus
    // comparison
    if (!r.read_u32(n)) return false;
    for (uint32_t i = 0; i < n; ++i) {
        std::string mpath;
        uint64_t mfp = 0;
        if (!r.read_str(mpath) || !r.read_u64(mfp)) return false;
    }
    if (r.p != r.end) return false;

    // ledger consistency: chunk_lengths is indexed by chunk_id
    if (lengths2.size() != chunks2.size()) return false;

    // restore the cheap derivations (contract save clause 2)
    for (Chunk& c : chunks2) {
        if (c.document_id < 0) return false;
        const size_t di = static_cast<size_t>(c.document_id);
        if (di >= docs2.size()) return false;
        const Document& d = docs2[di];
        // substr would throw std::out_of_range on bad offsets; the
        // contract forbids throwing
        if (c.begin > c.end || c.end > d.content.size()) return false;
        c.text = d.content.substr(c.begin, c.end - c.begin);
    }

    double avgdl = 0.0;
    if (!lengths2.empty()) {
        uint64_t sum = 0;
        for (int len : lengths2)
            sum += static_cast<uint64_t>(len);
        avgdl = static_cast<double>(sum) /
                static_cast<double>(lengths2.size());
    }

    // the all-in moment: outputs may only be touched after every step
    // above has succeeded
    docs = std::move(docs2);
    chunks = std::move(chunks2);
    index.postings = std::move(postings2);
    index.chunk_lengths = std::move(lengths2);
    index.avg_chunk_length = avgdl;
    return true;
}
