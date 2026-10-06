// vector_index.h -- exact dense retrieval, Flat backend (contract #10).
//
// Role: vector<float> in, ranked chunk ids out. Knows nothing about
// models, HTTP or JSON -- that is embedder.h's job and embedder.h is
// forbidden here (layering red line, embedder.h clause 7).
//
// Flat means brute force: one full scan per query, exact scores, no
// approximation. At 58 docs / 145 chunks a scan is sub-millisecond, so
// Flat is the practical default -- and it is simultaneously the exact
// ground truth that HNSW recall is measured against.
//
// Flat and HNSW (hnsw.h) are deliberately two independent contracts,
// not one abstract interface: swapping them is an experiment flag, not
// runtime injection (unlike embedder providers, which need fakes and
// an M4 replacement).

#pragma once

#include <vector>

#include "type.h"

// Flat storage follows the registry pattern used everywhere in this
// project: the vector index IS the chunk id (third application, after
// Document ids and Chunk ids).
struct FlatIndex {
    int dimension = 0;
    std::vector<std::vector<float>> vectors;  // vectors[i] = embedding of chunk i
};

// Build from the full chunk-vector list, in chunk_id order.
//
// 1. Empty input -> empty index (an empty corpus is legal; search then
//    returns an empty result).
// 2. Mixed dimensions -> throws std::runtime_error: that is a wiring
//    bug, not data, and must fail fast.
// 3. Vectors are stored as given. Unit length is guaranteed upstream by
//    the embedder contract and is NOT re-verified here.
FlatIndex build_flat_index(const std::vector<std::vector<float>>& vectors);

// Exact top-k search by cosine similarity.
//
// 1. Cosine is computed as a plain dot product: both sides are unit
//    length by the embedder contract, so no normalization happens here.
// 2. query.size() must equal index.dimension, else std::runtime_error
//    (again a wiring bug, fail fast).
// 3. Results are sorted score descending, ties chunk_id ascending
//    (the M1 search convention).
// 4. top_k <= 0 or an empty index -> an empty result (M1 guard
//    precedent).
// 5. Never throws otherwise; pure brute force, no state, no I/O.
std::vector<SearchResult>
search_flat(const FlatIndex& index, const std::vector<float>& query, int top_k);
