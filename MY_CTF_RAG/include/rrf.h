// rrf.h -- rank-only fusion of retrieval routes (contract #11).
//
// Role: fuse ranked lists from several retrieval routes (BM25, dense,
// ...) into one ranking. Ranks only, never raw scores: a BM25 score of
// 8.73 and a cosine of 0.81 live on different scales, so adding them
// is meaningless. Reciprocal Rank Fusion instead:
//
//     RRF(d) = sum over routes r containing d of  1 / (RRF_K + rank_r(d))
//
// k = 60 keeps the fusion stable and nearly parameter-free (fixed at
// project kick-off, AI-BRIEFING Q6; M5 may revisit).

#pragma once

#include <vector>

#include "type.h"

// Registered in docs/PARAMS.md. Zero tuning by design.
constexpr int RRF_K = 60;

// Fuse any number of ranked lists (M3 passes two: BM25 + dense).
//
// 1. Rank positions start at 1: the first entry of a list is rank 1.
// 2. A document's fused score is the sum of 1 / (RRF_K + rank) over
//    every list that contains it. A document absent from all lists is
//    absent from the output. Each list is expected to contain each
//    chunk_id at most once (both search routes guarantee this); if a
//    list repeats one, its best (smallest) rank counts.
// 3. Output is sorted fused score descending, ties chunk_id ascending.
//    The fused score is stored in SearchResult::score -- it is
//    dimensionless and must never be mixed with BM25 or cosine scores.
// 4. Empty input (no lists, or all lists empty) -> an empty output.
// 5. A single non-empty list degenerates to a re-scored passthrough of
//    itself.
// 6. Pure function: no I/O, no state, never throws.
std::vector<SearchResult>
rrf_fuse(const std::vector<std::vector<SearchResult>>& ranked_lists);
