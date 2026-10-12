// test_hnsw.cpp -- contract tests for hnsw.h (M3 step 6, contract #12).
// Recall is measured against Flat ground truth (vector_index.h), the
// acceptance criterion the contract names.

#include "check.h"
#include "hnsw.h"
#include "vector_index.h"

#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<float> vec(std::initializer_list<float> xs) {
    return std::vector<float>(xs);
}

bool order_is(const std::vector<SearchResult>& out,
              std::initializer_list<int> ids) {
    if (out.size() != ids.size())
        return false;
    std::size_t i = 0;
    for (int id : ids) {
        if (out[i].chunk_id != id)
            return false;
        ++i;
    }
    return true;
}

bool same_graph(const HnswIndex& a, const HnswIndex& b) {
    return a.entry_point == b.entry_point && a.max_level == b.max_level &&
           a.graph == b.graph;
}

bool same_results(const std::vector<SearchResult>& a,
                  const std::vector<SearchResult>& b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].chunk_id != b[i].chunk_id || a[i].score != b[i].score)
            return false;
    return true;
}

void normalize(std::vector<float>& v) {
    double norm = 0.0;
    for (float x : v)
        norm += static_cast<double>(x) * x;
    norm = std::sqrt(norm);
    if (norm == 0.0)
        return;
    for (float& x : v)
        x = static_cast<float>(static_cast<double>(x) / norm);
}

// Clustered synthetic corpus: `clusters` tight groups of `per_cluster`
// unit vectors in `dim` dimensions. Realistic for embeddings (nearby
// points cluster), and far friendlier to HNSW navigation than uniform
// random points, so the recall gate is stable.
std::vector<std::vector<float>> clustered(int dim, int clusters,
                                          int per_cluster, unsigned seed) {
    std::mt19937 gen(seed);
    std::normal_distribution<double> nd(0.0, 1.0);
    std::vector<std::vector<float>> out;
    for (int c = 0; c < clusters; ++c) {
        std::vector<double> center(static_cast<std::size_t>(dim));
        for (int d = 0; d < dim; ++d)
            center[static_cast<std::size_t>(d)] = nd(gen);
        double cn = 0.0;
        for (double x : center)
            cn += x * x;
        cn = std::sqrt(cn);
        for (double& x : center)
            x /= cn;
        for (int p = 0; p < per_cluster; ++p) {
            std::vector<float> v;
            v.reserve(static_cast<std::size_t>(dim));
            for (int d = 0; d < dim; ++d) {
                const double x =
                    center[static_cast<std::size_t>(d)] * 4.0 + nd(gen);
                v.push_back(static_cast<float>(x));
            }
            normalize(v);
            out.push_back(v);
        }
    }
    return out;
}

}  // namespace

int main() {
    // Case 1: same vectors + same seed -> identical graph and identical
    // search output (clause 1 reproducibility).
    {
        const std::vector<std::vector<float>> vs = {
            vec({1, 0, 0}), vec({0, 1, 0}), vec({0, 0, 1}),
            vec({0.5f, 0.5f, 0}), vec({0, 0.5f, 0.5f})};
        const HnswIndex a = build_hnsw(vs, 7u);
        const HnswIndex b = build_hnsw(vs, 7u);
        check(same_graph(a, b), "case1 same seed same graph");
        check(same_results(search_hnsw(a, vec({1, 0, 0}), 3),
                           search_hnsw(b, vec({1, 0, 0}), 3)),
              "case1 same seed same search");
    }

    // Case 2: empty corpus builds an empty legal index; search empty.
    {
        const HnswIndex idx = build_hnsw({}, 1u);
        check(idx.vectors.empty() && idx.dimension == 0 &&
                  idx.entry_point == -1 && idx.max_level == -1,
              "case2 empty build");
        check(search_hnsw(idx, vec({1, 0}), 5).empty(),
              "case2 search on empty index");
    }

    // Case 3: mixed dimensions at build -> throw.
    {
        bool threw = false;
        try {
            build_hnsw({vec({1, 0}), vec({1, 0, 0})}, 1u);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "case3 mixed dimensions throw");
    }

    // Case 4: top_k <= 0 returns empty (M1 guard precedent).
    {
        const HnswIndex idx = build_hnsw({vec({1, 0})}, 1u);
        check(search_hnsw(idx, vec({1, 0}), 0).empty(), "case4 k=0 empty");
        check(search_hnsw(idx, vec({1, 0}), -3).empty(), "case4 k=-3 empty");
    }

    // Case 5: top_k beyond N returns everything available, sorted.
    {
        const HnswIndex idx = build_hnsw({vec({1, 0}), vec({0, 1})}, 1u);
        const std::vector<SearchResult> out = search_hnsw(idx, vec({1, 0}), 99);
        check(order_is(out, {0, 1}), "case5 k=99 returns both, sorted");
    }

    // Case 6: query dimension mismatch -> throw (fail fast, as Flat).
    {
        const HnswIndex idx = build_hnsw({vec({1, 0})}, 1u);
        bool threw = false;
        try {
            search_hnsw(idx, vec({1, 0, 0}), 1);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "case6 query dim mismatch throws");
    }

    // Case 7: identical vectors tie, chunk_id ascending.
    {
        const HnswIndex idx = build_hnsw({vec({1, 0}), vec({1, 0})}, 1u);
        const std::vector<SearchResult> out = search_hnsw(idx, vec({1, 0}), 2);
        check(order_is(out, {0, 1}), "case7 tie-break id ascending");
    }

    // Case 8: Recall@10 against Flat ground truth on clustered data
    // (the contract's acceptance test), averaged over many queries.
    {
        const int dim = 16;
        const std::vector<std::vector<float>> vs = clustered(dim, 25, 8, 20261012u);
        const FlatIndex flat = build_flat_index(vs);
        const HnswIndex hnsw = build_hnsw(vs, 42u);

        std::mt19937 gen(999u);
        std::normal_distribution<double> nd(0.0, 1.0);
        int total = 0;
        int hits = 0;
        const int queries = 50;
        for (int q = 0; q < queries; ++q) {
            const std::size_t anchor =
                static_cast<std::size_t>(gen() % vs.size());
            std::vector<float> query = vs[anchor];
            for (float& x : query)
                x += static_cast<float>(nd(gen) * 0.2);
            normalize(query);

            const std::vector<SearchResult> truth = search_flat(flat, query, 10);
            const std::vector<SearchResult> got = search_hnsw(hnsw, query, 10);
            for (const SearchResult& t : truth) {
                ++total;
                for (const SearchResult& g : got)
                    if (g.chunk_id == t.chunk_id) {
                        ++hits;
                        break;
                    }
            }
        }
        const double recall = static_cast<double>(hits) / total;
        check(recall >= 0.85,
              "case8 recall@10 vs flat >= 0.85 (got " +
                  std::to_string(recall) + ")");
    }

    return test_summary();
}
