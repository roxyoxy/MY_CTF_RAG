#include "rrf.h"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::vector<SearchResult>
rrf_fuse(const std::vector<std::vector<SearchResult>>& ranked_lists) {
    std::unordered_map<int, double> acc;

    for (const std::vector<SearchResult>& list : ranked_lists) {
        std::unordered_set<int> seen_in_list;
        for (std::size_t pos = 0; pos < list.size(); ++pos) {
            // Clause 2: a repeated chunk_id counts once per list, at
            // its best (first) position; entries after a duplicate
            // keep their original offsets, nothing shifts up.
            if (!seen_in_list.insert(list[pos].chunk_id).second)
                continue;
            // Clause 1: ranks start at 1.
            acc[list[pos].chunk_id] +=
                1.0 / (RRF_K + static_cast<int>(pos) + 1);
        }
    }

    std::vector<SearchResult> out;
    out.reserve(acc.size());
    for (const auto& entry : acc)
        out.push_back(
            SearchResult{entry.first, static_cast<float>(entry.second)});

    // Clause 3: fused score descending, ties chunk_id ascending.
    // Scores compare for exact equality on purpose: golden case 1
    // relies on IEEE addition being commutative, so 1/61+1/62 and
    // 1/62+1/61 are bit-equal as doubles, and rounding the same
    // double to the stored float keeps them equal, so the
    // chunk_id tie-break stays observable.
    std::sort(out.begin(), out.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  if (a.score != b.score)
                      return a.score > b.score;
                  return a.chunk_id < b.chunk_id;
              });
    return out;
}
