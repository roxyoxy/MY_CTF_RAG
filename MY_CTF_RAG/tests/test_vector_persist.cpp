// test_vector_persist.cpp -- contract tests for vector_persist.h
// (M3 step 5, T9). Damage cases patch a good snapshot byte by byte
// and repair the header fingerprint, exercising the layer BEHIND the
// fingerprint. The offsets follow the format documented in
// vector_persist.h: 24-byte header, then the payload starting with
// the identity block (dimension u32, model_id str, embedding_policy
// str), then the document records.
//
// The three T9 knives (10-05 ruling) each have a home here:
//   knife 1: model_id patched + repaired fingerprint -> BAD_MODEL
//            (the identity layer catches it independently);
//   knife 2: dimension patched + repaired fingerprint -> BAD_FORMAT
//            (the structure walk derails: claimed dimension vs
//            actual vector bytes, before any identity comparison);
//   knife 3: a single vector float patched + repaired fingerprint
//            -> still loads (undetectable BY DESIGN: vectors have no
//            second source of truth; the defense relocated to the
//            caller refusing reuse on content_hash mismatch).
#include "check.h"
#include "vector_persist.h"

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
// exercises the layer BEHIND the fingerprint (test_persist precedent).
std::string repaired(const std::string& bytes) {
    std::string b = bytes;
    const uint64_t fp = fnv1a64(b.data() + 24, b.size() - 24);
    std::memcpy(&b[16], &fp, 8);
    return b;
}

struct VFixture {
    VectorIdentity identity;
    std::vector<DocVectors> docs;
};

VFixture make_fixture() {
    VFixture f;
    f.identity.model_id = "qwen3-embedding:0.6b";      // 20 chars
    f.identity.embedding_policy = "qwen3-instruct-v1"; // 16 chars
    f.identity.dimension = 4;

    DocVectors d0;
    d0.path = "pwn/one.md";
    d0.content_hash =
        vector_content_hash("ret2libc ROP chain with libc-2.31 gadgets");
    d0.vectors = { {1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f} };

    DocVectors d1;
    d1.path = "web/two.md";
    d1.content_hash =
        vector_content_hash("format string bug printf GOT overwrite");
    d1.vectors = { {0.f, 0.f, 1.f, 0.f} };

    f.docs = { d0, d1 };
    return f;
}

bool same_bytes(const std::vector<float>& a, const std::vector<float>& b) {
    return a.size() == b.size() &&
        std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
}

bool same_docvecs(const std::vector<DocVectors>& a,
                  const std::vector<DocVectors>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].path != b[i].path ||
            a[i].content_hash != b[i].content_hash ||
            a[i].vectors.size() != b[i].vectors.size())
            return false;
        for (size_t j = 0; j < a[i].vectors.size(); ++j)
            if (!same_bytes(a[i].vectors[j], b[i].vectors[j]))
                return false;
    }
    return true;
}

}  // namespace

int main() {
    const std::string snap = "test_vector_persist.bin";
    const std::string snap2 = "test_vector_persist2.bin";
    fs::remove(snap);
    fs::remove(snap2);
    fs::remove_all("test_vector_persist_io_dir");

    const VFixture fx = make_fixture();
    const VectorIdentity expected{ fx.identity.model_id,
                                   fx.identity.embedding_policy, 0 };

    // 1-2. good path: save, load, field equality at byte level
    check(vector_save(snap, fx.identity, fx.docs), "1 save ok");
    {
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::OK,
              "2a load ok");
        check(same_docvecs(fx.docs, out),
              "2b round trip field-equal (path, hash, vectors bitwise)");
    }

    // 3. byte identity: save -> load -> save
    {
        std::vector<DocVectors> out;
        const bool loaded =
            load_vector_snapshot(snap, expected, out) ==
            VectorSnapshotStatus::OK;
        check(loaded && vector_save(snap2, fx.identity, out),
              "3a load and re-save ok");
        check(slurp(snap) == slurp(snap2),
              "3b save->load->save byte-identical");
        fs::remove(snap2);
    }

    // One pristine copy: every damage case starts from GOOD bytes.
    const std::string good = slurp(snap);

    // 4. NOT_FOUND
    {
        std::vector<DocVectors> out;
        check(load_vector_snapshot("test_vector_persist_missing.bin",
                                   expected, out) ==
                  VectorSnapshotStatus::NOT_FOUND,
              "4 missing file -> NOT_FOUND");
    }

    // 5. IO_ERROR: path exists but is a directory
    fs::create_directories("test_vector_persist_io_dir");
    {
        std::vector<DocVectors> out;
        check(load_vector_snapshot("test_vector_persist_io_dir",
                                   expected, out) ==
                  VectorSnapshotStatus::IO_ERROR,
              "5 directory path -> IO_ERROR");
    }

    // 6. BAD_MAGIC: byte 0 flipped
    {
        std::string b = good;
        b[0] = static_cast<char>(b[0] ^ 0x20);
        spit(snap, b);
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_MAGIC,
              "6 magic byte flip -> BAD_MAGIC");
    }

    // 7. BAD_FORMAT: truncated below the header
    {
        std::string b = good;
        b.resize(10);
        spit(snap, b);
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "7 truncation -> BAD_FORMAT");
    }

    // 8. BAD_FORMAT: FORMAT_VERSION flipped (header not fingerprinted)
    {
        std::string b = good;
        b[8] = static_cast<char>(b[8] ^ 0xFF);
        spit(snap, b);
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "8 FORMAT byte flip -> BAD_FORMAT");
    }

    // 9. BAD_FORMAT: PIPELINE_VERSION flipped. Note the six-state
    // contract folds a pipeline mismatch into BAD_FORMAT -- there is
    // no separate BAD_PIPELINE here (unlike persist.h).
    {
        std::string b = good;
        b[12] = static_cast<char>(b[12] ^ 0xFF);
        spit(snap, b);
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "9 PIPELINE byte flip -> BAD_FORMAT");
    }

    // 10. BAD_FORMAT: payload byte flipped, fingerprint left stale
    {
        std::string b = good;
        b[100] = static_cast<char>(b[100] ^ 0x01);
        spit(snap, b);
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "10 payload flip, stale fingerprint -> BAD_FORMAT");
    }

    // 11. Knife 1: model_id patched (payload offset 8, file 32) with
    // a repaired fingerprint -- only the identity layer can catch it.
    {
        std::string b = good;
        b[32] = static_cast<char>(b[32] ^ 0x01);
        spit(snap, repaired(b));
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_MODEL,
              "11 knife 1: model_id patch + repaired fp -> BAD_MODEL");
    }

    // 12. Same knife aimed at embedding_policy (payload offset 32,
    // file 56)
    {
        std::string b = good;
        b[56] = static_cast<char>(b[56] ^ 0x01);
        spit(snap, repaired(b));
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_MODEL,
              "12 policy patch + repaired fp -> BAD_MODEL");
    }

    // 13. Knife 2: dimension patched (payload offset 0, file 24,
    // 4 -> 3) with a repaired fingerprint -- the structure walk
    // derails: stored vectors still carry 4 floats, the identity
    // block now claims 3. BAD_FORMAT fires BEFORE any identity
    // comparison, even though the model on disk matches expected.
    {
        std::string b = good;
        b[24] = static_cast<char>(3);
        spit(snap, repaired(b));
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "13 knife 2: dimension patch + repaired fp -> BAD_FORMAT");
    }

    // 14. Identity mismatch through the expected side: pristine file,
    // caller asks for a different dimension. dimension 0 in expected
    // means "do not compare" (startup cannot know it); a nonzero
    // expected dimension is enforced.
    {
        spit(snap, good);
        std::vector<DocVectors> out;
        const VectorIdentity wrong_dim{ fx.identity.model_id,
                                        fx.identity.embedding_policy,
                                        999 };
        check(load_vector_snapshot(snap, wrong_dim, out) ==
                  VectorSnapshotStatus::BAD_MODEL,
              "14 expected dimension 999 -> BAD_MODEL");
    }

    // 15. Knife 3: one float byte patched (1.0f -> 0.25f, top byte
    // 0x3F800000 -> 0x3E800000) with a repaired fingerprint -- loads
    // fine BY DESIGN. Stored vectors have no second source of truth;
    // the defense lives in the caller's content_hash reuse gate.
    // Pinned here so nobody "fixes" this into an accidental checksum
    // later.
    {
        std::string b = good;
        const char one_float_le[4] = { 0x00, 0x00, static_cast<char>(0x80),
                                       0x3F };
        const std::string needle(one_float_le, 4);
        const size_t pos = b.find(needle, 24);
        check(pos != std::string::npos, "15a knife 3 setup: float found");
        b[pos + 3] = static_cast<char>(b[pos + 3] ^ 0x01);  // 0x3F -> 0x3E
        spit(snap, repaired(b));
        std::vector<DocVectors> out;
        const VectorSnapshotStatus st =
            load_vector_snapshot(snap, expected, out);
        check(st == VectorSnapshotStatus::OK &&
                  !out.empty() &&
                  out[0].vectors[0][0] == 0.25f,
              "15b knife 3: float patch + repaired fp -> still OK "
              "(undetectable by design)");
    }

    // 16. empty corpus round trip
    {
        const VectorIdentity id4{ "m", "none", 4 };
        const std::vector<DocVectors> none;
        // 24 header + 4 dimension + (4+1) str "m" + (4+4) str "none"
        // + 4 doc count = 45
        check(vector_save(snap, id4, none) &&
                  slurp(snap).size() == 45,
              "16a empty save, header + identity + count only");
        std::vector<DocVectors> out;
        const VectorIdentity exp4{ "m", "none", 0 };
        check(load_vector_snapshot(snap, exp4, out) ==
                  VectorSnapshotStatus::OK && out.empty(),
              "16b empty snapshot loads OK");
    }

    // 17. determinism: reverse doc order in, identical bytes out
    {
        std::vector<DocVectors> reversed;
        reversed.push_back(fx.docs[1]);
        reversed.push_back(fx.docs[0]);
        check(vector_save(snap, fx.identity, reversed) &&
                  slurp(snap) == good,
              "17 reverse-order save byte-identical (path sort pins)");
    }

    // 18. structural flip (document count) with a repaired
    // fingerprint: the walk must derail on the third document
    {
        std::string b = good;
        b[72] = static_cast<char>(0x7F);
        b[73] = static_cast<char>(0xFF);
        b[74] = static_cast<char>(0xFF);
        b[75] = static_cast<char>(0xFF);
        spit(snap, repaired(b));
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "18 doc-count flip + repaired fp -> BAD_FORMAT");
    }

    // 19. trailing bytes with a repaired fingerprint
    {
        std::string b = good;
        b.push_back('x');
        spit(snap, repaired(b));
        std::vector<DocVectors> out;
        check(load_vector_snapshot(snap, expected, out) ==
                  VectorSnapshotStatus::BAD_FORMAT,
              "19 trailing byte + repaired fp -> BAD_FORMAT");
    }

    // 20. failure leaves the output untouched (all-or-nothing)
    {
        std::string b = good;
        b[32] = static_cast<char>(b[32] ^ 0x01);
        spit(snap, repaired(b));
        std::vector<DocVectors> probe = fx.docs;
        check(load_vector_snapshot(snap, expected, probe) ==
                  VectorSnapshotStatus::BAD_MODEL,
              "20a BAD_MODEL load fails");
        check(same_docvecs(fx.docs, probe),
              "20b outputs untouched after failure (probe kept)");
    }

    // 21. the exported hash helper: FNV-1a 64 semantics pinned
    {
        check(vector_content_hash("") == kFnvOffsetBasis,
              "21a empty input == FNV offset basis");
        check(vector_content_hash("ret2libc") ==
                  vector_content_hash("ret2libc"),
              "21b same input, same hash");
        check(vector_content_hash("ret2libc") !=
                  vector_content_hash("ret2libc!"),
              "21c one byte difference changes the hash");
    }

    fs::remove(snap);
    fs::remove_all("test_vector_persist_io_dir");
    return test_summary();
}
