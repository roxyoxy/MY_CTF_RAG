#include "vector_index.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

FlatIndex
build_flat_index(const std::vector<std::vector<float>>& vectors) {
    FlatIndex index;
    // Clause 1: an empty corpus is legal; search then returns empty.
    if (vectors.empty())
        return index;

    // Clause 2: mixed dimensions are a wiring bug, fail fast with the
    // offending indices spelled out.
    const std::size_t dimension = vectors[0].size();
    for (std::size_t i = 1; i < vectors.size(); ++i) {
        if (vectors[i].size() != dimension)
            throw std::runtime_error(
                "flat index: mixed vector dimensions: chunk " +
                std::to_string(i) + " has " +
                std::to_string(vectors[i].size()) + ", expected " +
                std::to_string(dimension));
    }

    // Clause 3: stored as given, no re-normalization, no unit-length
    // check -- unit length is the embedder contract's guarantee. The
    // signature passes a const reference, so the table is deep-copied
    // once (145 x 1024 floats ~ 0.6 MB, negligible at build time; the
    // card's "std::move the whole table" note overlooked the const&).
    index.dimension = static_cast<int>(dimension);
    index.vectors = vectors;
    return index;
}

std::vector<SearchResult>
search_flat(const FlatIndex& index, const std::vector<float>& query,
            int top_k) {
    // Clause 4: M1 guard precedent -- top_k <= 0 and an empty index
    // both return an empty result before anything else is checked.
    if (top_k <= 0)
        return {};
    if (index.vectors.empty())
        return {};
    // Clause 2: a dimension mismatch is a wiring bug, fail fast.
    if (query.size() != static_cast<std::size_t>(index.dimension))
        throw std::runtime_error(
            "flat search: query dimension " +
            std::to_string(query.size()) + " does not match index " +
            std::to_string(static_cast<std::size_t>(index.dimension)));

    // Clause 1: cosine as a plain dot product -- both sides arrive
    // unit length by the embedder contract, no normalization here.
    // Accumulation in double keeps 1024-term sums honest; the sum is
    // narrowed to the stored float score at the boundary.
    std::vector<SearchResult> out;
    out.reserve(index.vectors.size());
    for (std::size_t i = 0; i < index.vectors.size(); ++i) {
        const std::vector<float>& v = index.vectors[i];
        double dot = 0.0;
        for (std::size_t j = 0; j < query.size(); ++j)
            dot += static_cast<double>(query[j]) * v[j];
        out.push_back(
            SearchResult{static_cast<int>(i), static_cast<float>(dot)});
    }

    // Clause 3: score descending, ties chunk_id ascending (M1 search
    // convention; golden cases 1/2 pin the direction).
    std::sort(out.begin(), out.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  if (a.score != b.score)
                      return a.score > b.score;
                  return a.chunk_id < b.chunk_id;
              });

    out.resize(std::min(static_cast<std::size_t>(top_k), out.size()));
    return out;
}
