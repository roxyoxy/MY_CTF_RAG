// test_persist.cpp -- contract tests for persist.h (M2-2).
// Round trips through real snapshot files; damage cases patch a good
// snapshot byte by byte. The offsets used below follow the format
// documented in persist.h: 24-byte header (magic, FORMAT_VERSION,
// PIPELINE_VERSION, payload fingerprint), then the payload starting
// with CHUNK_SIZE and CHUNK_OVERLAP.
#include "check.h"
#include "persist.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr uint64_t kFnvOffsetBasis = 0xcbf29ce484222325ULL;
constexpr uint64_t kFnvPrime = 0x100000001b3ULL;

uint64_t fnv1a64(const char* data, size_t len) {
    uint64_t h = kFnvOffsetBasis;
    for (size_t i = 0; i < len; ++i) {
        h ^= static_cast<unsigned char>(data[i]);
        h *= kFnvPrime;
    }
    return h;
}

std::string slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}

void spit(const std::string& p, const std::string& bytes) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

// Repairs the header fingerprint after a payload edit so the test
// exercises the layer BEHIND the fingerprint.
std::string repaired(const std::string& bytes) {
    std::string b = bytes;
    const uint64_t fp = fnv1a64(b.data() + 24, b.size() - 24);
    std::memcpy(&b[16], &fp, 8);
    return b;
}

struct Fixture {
    std::vector<Document> docs;
    std::vector<Chunk> chunks;
    InvertedIndex index;
};

Fixture make_fixture() {
    Fixture f;
    Document d0;
    d0.id = 0;
    d0.path = "pwn/one.md";
    d0.content =
        "ret2libc ROP chain with libc-2.31 gadgets and a one-gadget fallback";
    f.docs.push_back(d0);
    Document d1;
    d1.id = 1;
    d1.path = "web/two.md";
    d1.content =
        "format string bug printf user input GOT overwrite at 127.0.0.1";
    d1.deleted = true;  // tombstone flag must survive the round trip
    f.docs.push_back(d1);

    Chunk c0;
    c0.id = 0;
    c0.document_id = 0;
    c0.begin = 0;
    c0.end = f.docs[0].content.size();
    c0.text = f.docs[0].content;
    f.chunks.push_back(c0);
    Chunk c1;
    c1.id = 1;
    c1.document_id = 1;
    c1.begin = f.docs[1].content.find("printf");
    c1.end = f.docs[1].content.size();
    c1.text = f.docs[1].content.substr(c1.begin, c1.end - c1.begin);
    f.chunks.push_back(c1);

    Posting p0;
    p0.chunk_id = 0;
    p0.tf = 1;
    f.index.postings["ret2libc"].push_back(p0);
    Posting p1;
    p1.chunk_id = 0;
    p1.tf = 1;
    f.index.postings["one-gadget"].push_back(p1);
    Posting p2;
    p2.chunk_id = 1;
    p2.tf = 2;
    f.index.postings["overwrite"].push_back(p2);
    f.index.chunk_lengths.push_back(12);
    f.index.chunk_lengths.push_back(9);
    f.index.avg_chunk_length = 10.5;
    return f;
}

bool same_docs(const std::vector<Document>& a, const std::vector<Document>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id || a[i].path != b[i].path ||
            a[i].content != b[i].content || a[i].deleted != b[i].deleted)
            return false;
    }
    return true;
}

bool same_chunks(const std::vector<Chunk>& a, const std::vector<Chunk>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id || a[i].document_id != b[i].document_id ||
            a[i].text != b[i].text || a[i].begin != b[i].begin ||
            a[i].end != b[i].end)
            return false;
    }
    return true;
}

bool same_index(const InvertedIndex& a, const InvertedIndex& b) {
    if (a.postings.size() != b.postings.size()) return false;
    for (const auto& kv : a.postings) {
        const auto it = b.postings.find(kv.first);
        if (it == b.postings.end()) return false;
        if (kv.second.size() != it->second.size()) return false;
        for (size_t j = 0; j < kv.second.size(); ++j) {
            if (kv.second[j].chunk_id != it->second[j].chunk_id ||
                kv.second[j].tf != it->second[j].tf)
                return false;
        }
    }
    return a.chunk_lengths == b.chunk_lengths &&
           a.avg_chunk_length == b.avg_chunk_length;
}

}  // namespace

int main() {
    const std::string snap = "test_persist.idx";
    const std::string snap2 = "test_persist2.idx";
    fs::remove(snap);
    fs::remove(snap2);
    fs::remove_all("test_persist_io_dir");

    const Fixture fx = make_fixture();

    // 1-3. the good path: save, validate, load, field equality
    check(save(snap, fx.docs, fx.chunks, fx.index), "1 save ok");
    check(validate(snap, fx.docs) == SnapshotStatus::OK, "2 validate ok");
    {
        std::vector<Document> ld;
        std::vector<Chunk> lc;
        InvertedIndex li;
        check(load(snap, ld, lc, li), "3a load ok");
        check(same_docs(fx.docs, ld) && same_chunks(fx.chunks, lc) &&
                  same_index(fx.index, li),
              "3b round trip field-equal (deleted flag, text, avgdl)");
    }

    // 4. byte identity: save -> load -> save (determinism)
    {
        std::vector<Document> ld;
        std::vector<Chunk> lc;
        InvertedIndex li;
        check(load(snap, ld, lc, li) && save(snap2, ld, lc, li),
              "4a load and re-save ok");
        check(slurp(snap) == slurp(snap2),
              "4b save->load->save byte-identical");
        fs::remove(snap2);
    }

    // 5. empty corpus round trip, avgdl guard
    {
        const std::vector<Document> none;
        const std::vector<Chunk> nch;
        const InvertedIndex nidx;
        check(save(snap, none, nch, nidx), "5a save empty corpus");
        check(validate(snap, none) == SnapshotStatus::OK,
              "5b validate empty corpus");
        std::vector<Document> ld;
        std::vector<Chunk> lc;
        InvertedIndex li;
        check(load(snap, ld, lc, li) && ld.empty() && lc.empty() &&
                  li.postings.empty() && li.chunk_lengths.empty() &&
                  li.avg_chunk_length == 0.0,
              "5c empty state restored, avgdl 0.0");
        // restore the good snapshot for the damage tests below
        check(save(snap, fx.docs, fx.chunks, fx.index), "5d re-save fixture");
    }

    // One pristine copy: every damage case below must start from GOOD
    // bytes, not from the previous case's wreckage.
    const std::string good = slurp(snap);

    // 6. NOT_FOUND
    check(validate("test_persist_missing.idx", fx.docs) ==
              SnapshotStatus::NOT_FOUND,
          "6 missing file -> NOT_FOUND");

    // 7. IO_ERROR: the path exists but is a directory, not readable
    // as a file
    fs::create_directories("test_persist_io_dir");
    check(validate("test_persist_io_dir", fx.docs) == SnapshotStatus::IO_ERROR,
          "7 directory path -> IO_ERROR");

    // 8. BAD_MAGIC: byte 0 flipped
    {
        std::string b = good;
        b[0] = static_cast<char>(b[0] ^ 0x20);
        spit(snap, b);
        check(validate(snap, fx.docs) == SnapshotStatus::BAD_MAGIC,
              "8 magic byte flip -> BAD_MAGIC");
    }

    // 9. BAD_FORMAT: FORMAT_VERSION byte flipped; the payload
    // fingerprint does not fire because the header is not part of it
    {
        std::string b = good;
        b[8] = static_cast<char>(b[8] ^ 0xFF);
        spit(snap, b);
        check(validate(snap, fx.docs) == SnapshotStatus::BAD_FORMAT,
              "9 FORMAT byte flip -> BAD_FORMAT");
    }

    // 10. BAD_FORMAT: truncated payload
    {
        std::string b = good;
        b.resize(b.size() * 2 / 5);
        spit(snap, b);
        check(validate(snap, fx.docs) == SnapshotStatus::BAD_FORMAT,
              "10 truncation -> BAD_FORMAT");
    }

    // 11. BAD_PIPELINE: byte 12 flipped
    {
        std::string b = good;
        b[12] = static_cast<char>(b[12] ^ 0xFF);
        spit(snap, b);
        check(validate(snap, fx.docs) == SnapshotStatus::BAD_PIPELINE,
              "11 PIPELINE byte flip -> BAD_PIPELINE");
    }

    // 12. BAD_PARAMS: CHUNK_SIZE patched in the payload AND the
    // fingerprint repaired, so only the params check can catch it
    {
        std::string b = good;
        b[24] = static_cast<char>(b[24] ^ 0x01);  // 500 -> 501
        spit(snap, repaired(b));
        check(validate(snap, fx.docs) == SnapshotStatus::BAD_PARAMS,
              "12 patched CHUNK_SIZE + repaired fingerprint -> BAD_PARAMS");
    }

    // 13. BAD_FORMAT: structural flip (document count) with a
    // repaired fingerprint; the structural walk must catch it
    {
        std::string b = good;
        b[32] = '\x7F';
        b[33] = '\xFF';
        b[34] = '\xFF';
        b[35] = '\xFF';
        spit(snap, repaired(b));
        check(validate(snap, fx.docs) == SnapshotStatus::BAD_FORMAT,
              "13 structural flip + repaired fingerprint -> BAD_FORMAT");
    }

    // 14. CORPUS_CHANGED: same file, different corpus bytes
    {
        spit(snap, good);
        check(validate(snap, fx.docs) == SnapshotStatus::OK,
              "14a good bytes restored");
        std::vector<Document> changed = fx.docs;
        changed[0].content += " extra line";
        check(validate(snap, changed) == SnapshotStatus::CORPUS_CHANGED,
              "14b corpus content change -> CORPUS_CHANGED");
    }

    // 15. failure leaves the outputs untouched (all-or-nothing)
    {
        std::vector<Document> probe = fx.docs;
        std::vector<Chunk> pc = fx.chunks;
        InvertedIndex pi = fx.index;
        check(!load("test_persist_missing.idx", probe, pc, pi),
              "15a load missing file fails");
        check(same_docs(fx.docs, probe) && same_chunks(fx.chunks, pc) &&
                  same_index(fx.index, pi),
              "15b outputs untouched after failure (probe kept)");
    }

    fs::remove(snap);
    fs::remove_all("test_persist_io_dir");
    return test_summary();
}
