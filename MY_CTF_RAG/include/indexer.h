#pragma once

// indexer.h
// Builds the inverted index and answers queries with BM25.

#include <string>
#include <unordered_map>
#include <vector>

#include "type.h"

// BM25 tuning knobs (defaults for M1, tuned in M5).
constexpr double BM25_K1 = 1.2;   // TF saturation
constexpr double BM25_B = 0.75;  // length normalization strength

// Default number of results returned by search.
constexpr int TOP_K = 10;

// One entry in a posting list.
struct Posting {
    int chunk_id;   // which chunk the term appears in
    int tf;         // how many times it appears there
};

// The ledger: term -> posting list, plus per-chunk lengths.
struct InvertedIndex {
    // term -> posting list; chunk ids ascending
    std::unordered_map<std::string, std::vector<Posting>> postings;

    // chunk_id -> word count (dl); index is the registry
    std::vector<int> chunk_lengths;

    // avgdl; 0.0 for an empty index
    double avg_chunk_length = 0.0;
};

// Builds the ledger from chunks.
//
// Contract:
// 1. Tokenizes every chunk.text with tokenize() from tokenizer.h
//    -- the same function queries use.
// 2. postings: for each term, a list of (chunk_id, tf);
//    chunk ids ascending.
// 3. chunk_lengths[chunk_id] = word count of that chunk.
// 4. avg_chunk_length = average over all chunks; 0.0 for empty input.
// 5. Empty input yields an empty index (all structures empty).
InvertedIndex build_index(const std::vector<Chunk>& chunks);

// BM25 search over the ledger.
//
// Contract:
// 1. Tokenizes the query with the same tokenize() -- both ends
//    share one tokenizer.
// 2. Scores candidate chunks with BM25:
//      score(q, d) = sum over query terms t:
//          IDF(t) * tf * (k1 + 1)
//          / (tf + k1 * (1 - b + b * dl / avgdl))
//      IDF(t) = ln(1 + (N - df + 0.5) / (df + 0.5))
// 3. Query terms not present in the index are skipped.
// 4. Results sorted by score descending; ties broken by
//    chunk_id ascending.
// 5. At most top_k results are returned.
// 6. An empty query or an empty index yields an empty vector.
std::vector<SearchResult> search(
    const InvertedIndex& index,
    const std::string& query,
    int top_k = TOP_K);