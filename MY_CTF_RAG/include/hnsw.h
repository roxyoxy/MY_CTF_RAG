// hnsw.h -- self-built approximate nearest neighbor search (contract #12).
//
// Role: a layered proximity graph over the same chunk vectors. Flat is
// exact and stays the product default at 58 docs / 145 chunks; HNSW is
// the algorithm-core piece and the M5 experiment object -- its recall
// is only meaningful measured against Flat (vector_index.h). Its value
// threshold is roughly 10^5..10^6 vectors; below that Flat wins on
// simplicity and exactness.
//
// Construction order note: this contract is designed now but
// implemented in M3 step 5, the Go/No-Go review point -- it may slip
// without blocking any delivered capability.

#pragma once

#include <vector>

#include "type.h"

// The three HNSW knobs, registered in docs/PARAMS.md (M5 tunes them).
constexpr int HNSW_M = 16;                 // max links per node per layer (graph connectivity)
constexpr int HNSW_EF_CONSTRUCTION = 200;  // candidate width while building (build quality)
constexpr int HNSW_EF_SEARCH = 100;        // beam width while querying (the recall-latency dial)

// What the contract pins; everything else is implementation freedom to
// be spelled out on the task card:
// 1. vectors[i] is the embedding of chunk i -- the same registry
//    pattern as FlatIndex; chunk ids are the currency everywhere.
// 2. Deterministic: build_hnsw on the same vectors with the same seed
//    produces the same graph. Tests rely on this.
struct HnswIndex {
    int dimension = 0;
    std::vector<std::vector<float>> vectors;
    // Layered adjacency (graph[level][node] = neighbor ids), entry
    // point and max level: fields and comments added by the implementer
    // on the task card; adding them is layout, not a signature change.
};

// Build the layered graph.
//
// 1. Random layer assignment draws from the provided seed:
//    reproducibility is a contract, not a courtesy.
// 2. Empty input -> an empty index. Mixed dimensions ->
//    std::runtime_error (wiring bug, same policy as build_flat_index).
HnswIndex build_hnsw(const std::vector<std::vector<float>>& vectors, unsigned seed);

// Approximate top-k search.
//
// 1. Same result conventions as search_flat: score descending, ties
//    chunk_id ascending, top_k <= 0 or an empty index -> an empty
//    result. Similarity is the plain dot product (unit vectors by the
//    embedder contract).
// 2. Approximate by design: recall is NOT guaranteed to be 1.0. The
//    acceptance test is Recall@10 against Flat ground truth.
// 3. Uses HNSW_EF_SEARCH internally; never throws.
std::vector<SearchResult>
search_hnsw(const HnswIndex& index, const std::vector<float>& query, int top_k);
