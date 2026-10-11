// vector_persist.cpp -- vector.bin snapshot (13th contract,
// include/vector_persist.h).
//
// Implementation mirrors the M2 persist.cpp pattern: fixed-width
// fields at native byte order, the payload assembled fully in memory,
// an FNV-1a header fingerprint over every payload byte, and an atomic
// .tmp rename on save. The snapshot is a local cache, not an exchange
// format.
//
// Detection boundary, stated honestly: the fingerprint defends
// against accidental corruption, not against a deliberate edit with
// a repaired fingerprint. A single vector's float bytes changed and
// the fingerprint recomputed still loads (T9 ruling, third knife) --
// stored vectors have no second source of truth to audit against
// (index.bin audits through the corpus files; vector.bin has none).
// The relocated defense is the caller's reuse rule: a content_hash
// mismatch refuses reuse, so an edited document always re-embeds.

#include "vector_persist.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

namespace {

constexpr size_t kHeaderSize = 24;

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
// Same reason as persist.cpp: the header fingerprint covers every
// payload byte, so it can only be computed after the payload exists,
// yet the header sits at the front of the file.

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

void append_vector(std::string& buf, const std::vector<float>& v)
{
    append_u32(buf, static_cast<uint32_t>(v.size()));
    char tmp[4];
    for (float f : v) {
        std::memcpy(tmp, &f, sizeof tmp);
        buf.append(tmp, sizeof tmp);
    }
}

// ---- read family: a bounds-checked cursor, same as persist.cpp ----
// Never throws implies never trusts: every read validates the
// remaining byte count first, and counts from disk never feed a
// reserve() (a forged count plus reserve is a bad_alloc).

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
        if (static_cast<uint64_t>(len) > static_cast<uint64_t>(end - p))
            return false;
        s.assign(p, len);
        p += len;
        return true;
    }
};

bool read_file(const std::string& path, std::string& buf)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char block[8192];
    while (in.read(block, sizeof block) || in.gcount() > 0)
        buf.append(block, static_cast<size_t>(in.gcount()));
    return !in.bad();
}

}  // namespace

std::uint64_t vector_content_hash(const std::string& content)
{
    return fnv1a64(content.data(), content.size());
}

bool vector_save(const std::string& path,
                 const VectorIdentity& identity,
                 const std::vector<DocVectors>& docs)
{
    std::string payload;

    append_u32(payload, static_cast<uint32_t>(identity.dimension));
    append_str(payload, identity.model_id);
    append_str(payload, identity.embedding_policy);

    // Ascending path order pins the bytes: the same vector set saved
    // twice yields identical files whatever the caller's doc order
    // (deterministic serialization, save->load->save equality).
    std::vector<const DocVectors*> ordered;
    ordered.reserve(docs.size());
    for (const DocVectors& d : docs)
        ordered.push_back(&d);
    std::sort(ordered.begin(), ordered.end(),
        [](const DocVectors* a, const DocVectors* b) {
            return a->path < b->path;
        });

    append_u32(payload, static_cast<uint32_t>(ordered.size()));
    for (const DocVectors* d : ordered) {
        append_str(payload, d->path);
        append_u64(payload, d->content_hash);
        append_u32(payload, static_cast<uint32_t>(d->vectors.size()));
        for (const std::vector<float>& v : d->vectors)
            append_vector(payload, v);
    }

    const uint64_t fingerprint = fnv1a64(payload.data(), payload.size());

    std::string file;
    file.reserve(kHeaderSize + payload.size());
    file.append(VECTOR_MAGIC, sizeof VECTOR_MAGIC);
    append_u32(file, static_cast<uint32_t>(VECTOR_FORMAT_VERSION));
    append_u32(file, static_cast<uint32_t>(VECTOR_PIPELINE_VERSION));
    append_u64(file, fingerprint);
    file.append(payload);

    // Atomic replace, same two-step fallback as persist.cpp.
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

VectorSnapshotStatus load_vector_snapshot(const std::string& path,
    const VectorIdentity& expected,
    std::vector<DocVectors>& out)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return VectorSnapshotStatus::NOT_FOUND;

    std::string buf;
    if (!read_file(path, buf))
        return VectorSnapshotStatus::IO_ERROR;

    // Fixed check order, part of the contract: magic -> versions ->
    // fingerprint -> structure walk -> identity.
    if (buf.size() >= 8 &&
        std::memcmp(buf.data(), VECTOR_MAGIC, sizeof VECTOR_MAGIC) != 0)
        return VectorSnapshotStatus::BAD_MAGIC;
    if (buf.size() < kHeaderSize)
        return VectorSnapshotStatus::BAD_FORMAT;
    uint32_t fmt = 0;
    uint32_t pipe = 0;
    uint64_t stored_fp = 0;
    std::memcpy(&fmt, buf.data() + 8, 4);
    std::memcpy(&pipe, buf.data() + 12, 4);
    std::memcpy(&stored_fp, buf.data() + 16, 8);
    if (fmt != VECTOR_FORMAT_VERSION)
        return VectorSnapshotStatus::BAD_FORMAT;
    if (fnv1a64(buf.data() + kHeaderSize, buf.size() - kHeaderSize)
        != stored_fp)
        return VectorSnapshotStatus::BAD_FORMAT;
    if (pipe != VECTOR_PIPELINE_VERSION)
        return VectorSnapshotStatus::BAD_FORMAT;

    Reader r{ buf.data() + kHeaderSize, buf.data() + buf.size() };

    // Identity block as stored on disk. The comparison against the
    // expected identity runs only after the whole structure walked
    // clean (contract clause 2).
    uint32_t dim = 0;
    std::string model;
    std::string policy;
    if (!r.read_u32(dim) || !r.read_str(model) || !r.read_str(policy))
        return VectorSnapshotStatus::BAD_FORMAT;

    // Structure walk into local temporaries: outputs stay untouched
    // until the identity check has also passed (all-or-nothing).
    std::vector<DocVectors> docs2;
    uint32_t n = 0;
    if (!r.read_u32(n))
        return VectorSnapshotStatus::BAD_FORMAT;
    for (uint32_t i = 0; i < n; ++i) {
        DocVectors d;
        if (!r.read_str(d.path) || !r.read_u64(d.content_hash))
            return VectorSnapshotStatus::BAD_FORMAT;
        uint32_t m = 0;
        if (!r.read_u32(m))
            return VectorSnapshotStatus::BAD_FORMAT;
        for (uint32_t j = 0; j < m; ++j) {
            uint32_t len = 0;
            if (!r.read_u32(len))
                return VectorSnapshotStatus::BAD_FORMAT;
            // Internal consistency: every stored vector must carry
            // exactly the dimension the identity block claims.
            if (len != dim)
                return VectorSnapshotStatus::BAD_FORMAT;
            if (static_cast<uint64_t>(len) * 4ULL >
                static_cast<uint64_t>(r.end - r.p))
                return VectorSnapshotStatus::BAD_FORMAT;
            std::vector<float> v(len);
            std::memcpy(v.data(), r.p, static_cast<size_t>(len) * 4);
            r.p += static_cast<size_t>(len) * 4;
            d.vectors.push_back(std::move(v));
        }
        docs2.push_back(std::move(d));
    }
    if (r.p != r.end)
        return VectorSnapshotStatus::BAD_FORMAT;  // trailing bytes

    // Identity last. model_id and embedding_policy are always
    // compared. dimension == 0 in expected means "do not compare":
    // callers learn the dimension from the provider's output, not
    // from the provider (embedder.h clause 5), so at startup only the
    // model and policy are known -- and the model id already pins the
    // coordinate system. A nonzero expected dimension (tests, or a
    // caller that already knows it) is still enforced.
    if (model != expected.model_id ||
        policy != expected.embedding_policy)
        return VectorSnapshotStatus::BAD_MODEL;
    if (expected.dimension != 0 &&
        static_cast<int>(dim) != expected.dimension)
        return VectorSnapshotStatus::BAD_MODEL;

    out = std::move(docs2);
    return VectorSnapshotStatus::OK;
}
