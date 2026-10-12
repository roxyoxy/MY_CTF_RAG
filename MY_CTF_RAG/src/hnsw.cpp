// hnsw.cpp -- layered approximate nearest-neighbor search (contract #12).
//
// Standard HNSW (Malkov & Yashunin): a node's level is a geometric draw
// (p = 1 / HNSW_M), the upper layers form a sparse navigable skeleton,
// and level 0 is the full graph. Search descends greedily to the query's
// neighborhood, then runs an HNSW_EF_SEARCH beam at level 0. Approximate
// by design; recall is only meaningful measured against Flat
// (vector_index.h) ground truth.
//
// Similarity is the plain dot product (unit vectors upstream by the
// embedder contract), so "closer" means a higher dot throughout.

#include "hnsw.h"

#include <algorithm>
#include <cstddef>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Cand {
    double sim;
    int id;
};

// Max-heap by similarity (top = highest dot = closest). Ties broken by
// smaller id first so heap order stays deterministic across runs.
struct MaxSim {
    bool operator()(const Cand& a, const Cand& b) const {
        if (a.sim != b.sim)
            return a.sim < b.sim;
        return a.id > b.id;
    }
};

// Min-heap by similarity (top = lowest dot = farthest). Ties broken by
// smaller id first.
struct MinSim {
    bool operator()(const Cand& a, const Cand& b) const {
        if (a.sim != b.sim)
            return a.sim > b.sim;
        return a.id > b.id;
    }
};

double dot(const std::vector<float>& a, const std::vector<float>& b) {
    double s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
        s += static_cast<double>(a[i]) * b[i];
    return s;
}

const std::vector<int>& neighbors(const HnswIndex& idx, int level, int node) {
    return idx.graph[static_cast<std::size_t>(level)]
                    [static_cast<std::size_t>(node)];
}

std::vector<int>& neighbors(HnswIndex& idx, int level, int node) {
    return idx.graph[static_cast<std::size_t>(level)]
                    [static_cast<std::size_t>(node)];
}

// Greedy hill-climb at one layer: walk to the highest-dot neighbor until
// no neighbor improves. Deterministic (strict > keeps ties stationary).
int greedy_at_level(const HnswIndex& idx, const std::vector<float>& query,
                    int start, int level) {
    int cur = start;
    double cur_sim = dot(query, idx.vectors[static_cast<std::size_t>(cur)]);
    bool improved = true;
    while (improved) {
        improved = false;
        for (int nb : neighbors(idx, level, cur)) {
            const double s =
                dot(query, idx.vectors[static_cast<std::size_t>(nb)]);
            if (s > cur_sim) {
                cur_sim = s;
                cur = nb;
                improved = true;
            }
        }
    }
    return cur;
}

// Beam search at one layer: returns up to ef closest nodes (by dot),
// sorted dot descending, ties id ascending.
std::vector<Cand> search_layer(const HnswIndex& idx,
                               const std::vector<float>& query,
                               const std::vector<int>& entries, int ef,
                               int level) {
    const int n = static_cast<int>(idx.vectors.size());
    std::vector<bool> visited(static_cast<std::size_t>(n), false);
    std::priority_queue<Cand, std::vector<Cand>, MaxSim> candidates;
    std::priority_queue<Cand, std::vector<Cand>, MinSim> results;

    for (int e : entries) {
        if (visited[static_cast<std::size_t>(e)])
            continue;
        visited[static_cast<std::size_t>(e)] = true;
        const double s = dot(query, idx.vectors[static_cast<std::size_t>(e)]);
        candidates.push(Cand{s, e});
        results.push(Cand{s, e});
    }

    while (!candidates.empty()) {
        const Cand c = candidates.top();
        candidates.pop();
        // If the closest unexpanded node is farther than the farthest
        // kept result, no remaining node can improve the ef-set.
        if (static_cast<int>(results.size()) >= ef && c.sim < results.top().sim)
            break;
        for (int nb : neighbors(idx, level, c.id)) {
            if (visited[static_cast<std::size_t>(nb)])
                continue;
            visited[static_cast<std::size_t>(nb)] = true;
            const double s =
                dot(query, idx.vectors[static_cast<std::size_t>(nb)]);
            if (static_cast<int>(results.size()) < ef || s > results.top().sim) {
                candidates.push(Cand{s, nb});
                results.push(Cand{s, nb});
                if (static_cast<int>(results.size()) > ef)
                    results.pop();
            }
        }
    }

    std::vector<Cand> out;
    out.reserve(results.size());
    while (!results.empty()) {
        out.push_back(results.top());
        results.pop();
    }
    std::sort(out.begin(), out.end(), [](const Cand& a, const Cand& b) {
        if (a.sim != b.sim)
            return a.sim > b.sim;
        return a.id < b.id;
    });
    return out;
}

// Trim a node's layer-neighbor list to the HNSW_M highest-dot neighbors.
void prune(HnswIndex& idx, int node, int level) {
    std::vector<int>& list = neighbors(idx, level, node);
    if (static_cast<int>(list.size()) <= HNSW_M)
        return;
    const std::vector<float>& v = idx.vectors[static_cast<std::size_t>(node)];
    std::sort(list.begin(), list.end(), [&](int a, int b) {
        const double sa = dot(v, idx.vectors[static_cast<std::size_t>(a)]);
        const double sb = dot(v, idx.vectors[static_cast<std::size_t>(b)]);
        if (sa != sb)
            return sa > sb;
        return a < b;
    });
    list.resize(static_cast<std::size_t>(HNSW_M));
}

// Bidirectionally connect node to its closest found neighbors at one
// layer, then prune every touched list back to HNSW_M.
void connect_and_prune(HnswIndex& idx, int node, int level,
                       const std::vector<Cand>& found) {
    const int m = std::min(HNSW_M, static_cast<int>(found.size()));
    for (int k = 0; k < m; ++k) {
        const int nb = found[static_cast<std::size_t>(k)].id;
        neighbors(idx, level, node).push_back(nb);
        neighbors(idx, level, nb).push_back(node);
    }
    prune(idx, node, level);
    for (int k = 0; k < m; ++k)
        prune(idx, found[static_cast<std::size_t>(k)].id, level);
}

}  // namespace

HnswIndex build_hnsw(const std::vector<std::vector<float>>& vectors,
                     unsigned seed) {
    HnswIndex idx;
    // Clause 2a: an empty corpus is legal; search then returns empty.
    if (vectors.empty())
        return idx;

    // Clause 2b: mixed dimensions are a wiring bug, fail fast.
    const std::size_t dimension = vectors[0].size();
    for (std::size_t i = 1; i < vectors.size(); ++i) {
        if (vectors[i].size() != dimension)
            throw std::runtime_error(
                "hnsw build: mixed vector dimensions: chunk " +
                std::to_string(i) + " has " +
                std::to_string(vectors[i].size()) + ", expected " +
                std::to_string(dimension));
    }

    idx.dimension = static_cast<int>(dimension);
    idx.vectors = vectors;
    const int n = static_cast<int>(vectors.size());

    // Clause 1: random layer assignment draws from the provided seed;
    // reproducibility is a contract, not a courtesy. A geometric draw
    // with p = 1 / HNSW_M matches the paper's P(level >= l) = M^-l.
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<int> node_level(static_cast<std::size_t>(n), 0);
    int max_level = 0;
    for (int i = 0; i < n; ++i) {
        int lvl = 0;
        while (dist(gen) < 1.0 / HNSW_M)
            ++lvl;
        node_level[static_cast<std::size_t>(i)] = lvl;
        if (lvl > max_level)
            max_level = lvl;
    }

    idx.max_level = max_level;
    idx.graph.assign(
        static_cast<std::size_t>(max_level + 1),
        std::vector<std::vector<int>>(static_cast<std::size_t>(n)));

    // Node 0 seeds the graph; its own layers stay empty.
    idx.entry_point = 0;
    int entry_level = node_level[0];

    for (int i = 1; i < n; ++i) {
        const int level = node_level[static_cast<std::size_t>(i)];
        const std::vector<float>& q = idx.vectors[static_cast<std::size_t>(i)];
        int cur = idx.entry_point;

        if (level <= entry_level) {
            // Descend from the entry layer to just above the new node,
            // then connect downward layer by layer.
            for (int l = entry_level; l >= level + 1; --l)
                cur = greedy_at_level(idx, q, cur, l);
            for (int l = level; l >= 0; --l) {
                const std::vector<Cand> found =
                    search_layer(idx, q, std::vector<int>{cur},
                                 HNSW_EF_CONSTRUCTION, l);
                connect_and_prune(idx, i, l, found);
                if (!found.empty())
                    cur = found[0].id;
            }
        } else {
            // The new node tops the hierarchy: nothing to descend above
            // the old entry layer; it is alone there and becomes entry.
            for (int l = entry_level; l >= 0; --l) {
                const std::vector<Cand> found =
                    search_layer(idx, q, std::vector<int>{cur},
                                 HNSW_EF_CONSTRUCTION, l);
                connect_and_prune(idx, i, l, found);
                if (!found.empty())
                    cur = found[0].id;
            }
            idx.entry_point = i;
            entry_level = level;
        }
    }

    return idx;
}

std::vector<SearchResult>
search_hnsw(const HnswIndex& index, const std::vector<float>& query,
            int top_k) {
    // Clause 1 guards: same conventions as search_flat (M1 precedent).
    if (top_k <= 0)
        return {};
    if (index.vectors.empty())
        return {};

    // A dimension mismatch is a wiring bug and fails fast, exactly as
    // search_flat clause 2 -- reading out of bounds is worse than
    // throwing. Clause 3's "never throws" holds for a well-formed query;
    // this guard is the same fail-fast policy the sibling contract uses.
    if (static_cast<int>(query.size()) != index.dimension)
        throw std::runtime_error(
            "hnsw search: query dimension " +
            std::to_string(query.size()) + " does not match index " +
            std::to_string(static_cast<std::size_t>(index.dimension)));

    // Descend the sparse upper layers, then beam search level 0.
    int cur = index.entry_point;
    for (int l = index.max_level; l >= 1; --l)
        cur = greedy_at_level(index, query, cur, l);

    const std::vector<Cand> found =
        search_layer(index, query, std::vector<int>{cur}, HNSW_EF_SEARCH, 0);

    std::vector<SearchResult> out;
    const std::size_t limit =
        std::min(static_cast<std::size_t>(top_k), found.size());
    out.reserve(limit);
    for (std::size_t k = 0; k < limit; ++k)
        out.push_back(
            SearchResult{found[k].id, static_cast<float>(found[k].sim)});
    return out;
}
