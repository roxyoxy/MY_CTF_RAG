// test_indexer.cpp -- contract tests for indexer.h (T4).
// Covers the ledger format, BM25 double math, tie-break, and edges.
#include "check.h"
#include "indexer.h"
#include "type.h"
#include <cmath>
#include <string>
#include <vector>

static Chunk makeChunk(int id, int docId, const std::string& text) {
    Chunk c;
    c.id = id;
    c.document_id = docId;
    c.text = text;
    c.begin = 0;
    c.end = static_cast<int>(text.size());
    return c;
}

int main() {
    // Ledger: chunk_lengths indexed by chunk_id.
    std::vector<Chunk> ledger;
    ledger.push_back(makeChunk(0, 0, "alpha"));
    ledger.push_back(makeChunk(1, 0, "beta"));
    ledger.push_back(makeChunk(2, 0, "alpha alpha"));
    const InvertedIndex idx = build_index(ledger);
    check(idx.chunk_lengths.size() == 3 && idx.chunk_lengths[0] == 1 &&
              idx.chunk_lengths[1] == 1 && idx.chunk_lengths[2] == 2,
          "1 chunk_lengths indexed by chunk_id");

    // avgdl must be double math (integer-division trap: 4/3 vs 1).
    check(std::abs(idx.avg_chunk_length - 4.0 / 3.0) < 1e-9,
          "2 avgdl = 4/3 in double precision");

    // Postings ascending by chunk_id with correct tf.
    const auto& pl = idx.postings.at("alpha");
    check(pl.size() == 2 && pl[0].chunk_id == 0 && pl[0].tf == 1 &&
              pl[1].chunk_id == 2 && pl[1].tf == 2,
          "3 postings ascending by chunk_id, tf correct");

    // Equal scores -> chunk_id ascending (contract clause 4).
    std::vector<Chunk> tie;
    tie.push_back(makeChunk(0, 0, "x"));
    tie.push_back(makeChunk(1, 0, "x"));
    tie.push_back(makeChunk(2, 0, "x"));
    const auto r3 = search(build_index(tie), "x", 5);
    check(r3.size() == 3 && r3[0].chunk_id == 0 && r3[1].chunk_id == 1 &&
              r3[2].chunk_id == 2 && r3[0].score == r3[1].score,
          "4 equal scores -> chunk_id ascending");

    // tf saturation: tf=2 ranks above tf=1.
    const auto r4 = search(idx, "alpha", 5);
    check(r4.size() == 2 && r4[0].chunk_id == 2 && r4[1].chunk_id == 0 &&
              r4[0].score > r4[1].score,
          "5 tf=2 chunk ranks above tf=1");

    // Edge behaviour.
    check(search(idx, "zzz-not-in-index", 5).empty(),
          "6 unknown term -> empty");
    check(search(idx, "...", 5).empty(),
          "7 all-separator query -> empty");
    check(search(idx, "alpha", 1).size() == 1,
          "8 top_k clamps result size");
    check(search(idx, "alpha", 0).empty() && search(idx, "alpha", -3).empty(),
          "9 non-positive top_k -> empty (round-4 guard)");
    check(search(build_index({}), "alpha", 5).empty(),
          "10 empty index -> empty");

    // End-to-end mini corpus; both ends share the same tokenizer.
    std::vector<Chunk> mini;
    mini.push_back(makeChunk(
        0, 0, "ret2libc exploit requires leaking the libc base address"));
    mini.push_back(makeChunk(
        1, 0, "sql injection in the login form bypasses authentication"));
    mini.push_back(makeChunk(
        2, 0, "ret2libc needs a leak of libc base and a return-oriented chain"));
    const auto r6 = search(build_index(mini), "ret2libc libc base", 3);
    check(r6.size() == 2 &&
              (r6[0].chunk_id == 0 || r6[0].chunk_id == 2) &&
              (r6[1].chunk_id == 0 || r6[1].chunk_id == 2),
          "11 end-to-end query hits only ret2libc chunks");

    return test_summary();
}
