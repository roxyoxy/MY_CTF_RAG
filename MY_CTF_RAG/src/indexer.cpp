// indexer.cpp
// Implements the contract in include/indexer.h.

#include "indexer.h"
#include "tokenizer.h"
#include <algorithm>  // std::sort, std::min
#include <cmath>      // std::log
#include <unordered_map>
#include <vector>

InvertedIndex build_index(const std::vector<Chunk>& chunks) {
    InvertedIndex index;

    for (const auto& c : chunks) {
        // Tokenize with the same function queries use (contract clause 1)
        std::vector<std::string> tokens = tokenize(c.text);

        // Record chunk length; push_back is safe because ids are
        // globally continuous and ascending from 0 (chunker contract)
        index.chunk_lengths.push_back(static_cast<int>(tokens.size()));

        // Count term frequencies within this chunk
        std::unordered_map<std::string, int> tf_counts;
        for (const auto& t : tokens) {
            tf_counts[t]++;
        }

        // Append to posting lists; chunk_id ascending comes for free
        // because we iterate chunks in order
        for (const auto& [term, tf] : tf_counts) {
            index.postings[term].push_back({ c.id, tf });
        }
    }

    // Compute average chunk length; avoid integer division
    int N = static_cast<int>(index.chunk_lengths.size());
    if (N == 0) {
        index.avg_chunk_length = 0.0;
    }
    else {
        double total = 0.0;
        for (int len : index.chunk_lengths) {
            total += static_cast<double>(len);
        }
        index.avg_chunk_length = total / static_cast<double>(N);
    }

    return index;
}

std::vector<SearchResult> search(
    const InvertedIndex& index,
    const std::string& query,
    int top_k) {

    // Guard: a non-positive top_k can only be a caller bug; returning
    // early also keeps the negative value away from resize() below,
    // where it would convert to a huge size_t (round-4 fix)
    if (top_k <= 0) {
        return {};
    }

    int N = static_cast<int>(index.chunk_lengths.size());
    if (N == 0) {
        return {};
    }

    // Tokenize query with the same function used at index time
    std::vector<std::string> qtokens = tokenize(query);
    if (qtokens.empty()) {
        return {};
    }

    // Accumulator: chunk_id -> raw BM25 score (double for precision)
    std::unordered_map<int, double> scores;

    for (const auto& t : qtokens) {
        auto it = index.postings.find(t);
        if (it == index.postings.end()) {
            continue; // term not in index, skip (contract clause 3)
        }

        const auto& posting_list = it->second;
        int df = static_cast<int>(posting_list.size());
        double idf = std::log(1.0 + (N - df + 0.5) / (df + 0.5));

        for (const auto& p : posting_list) {
            int cid = p.chunk_id;
            int tf = p.tf;
            int dl = index.chunk_lengths[cid];
            double avgdl = index.avg_chunk_length;

            double numerator = idf * tf * (BM25_K1 + 1.0);
            double denominator = tf + BM25_K1 * (1.0 - BM25_B + BM25_B * dl / avgdl);
            scores[cid] += numerator / denominator;
        }
    }

    // Materialize results into a vector for sorting
    std::vector<SearchResult> results;
    for (const auto& [cid, score] : scores) {
        SearchResult sr;
        sr.chunk_id = cid;
        sr.score = static_cast<float>(score); // avoid C4244 narrowing warning
        results.push_back(sr);
    }

    // Sort: score descending; tie-break by chunk_id ascending
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            if (a.score != b.score) {
                return a.score > b.score;
            }
            return a.chunk_id < b.chunk_id;
        });

    // Truncate to top_k, guard against top_k > results.size()
    int k = std::min(top_k, static_cast<int>(results.size()));
    results.resize(k);

    return results;
}