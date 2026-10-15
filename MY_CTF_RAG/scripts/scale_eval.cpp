// scale_eval.cpp -- M5 scale-ladder evaluation driver (scripts/ tool).
//
// Runs the REAL product modules (loader / chunker / indexer / Flat /
// HNSW / RRF / embedder / vector_persist) over a corpus directory and
// a frozen query set, and writes per-query route rankings + latencies
// as JSONL. Scoring (MRR etc.) happens in Python; this tool only
// measures and reports.
//
// Real-corpus mode:
//   scale_eval.exe --data <dir> --queries <q.jsonl> --out <out.jsonl>
//                  [--vectors <vector.bin>] [--seed 42]
// Synthetic mode (HNSW-vs-Flat crossover, no text side):
//   scale_eval.exe --synthetic <N> --out <out.jsonl> [--dim 1024]
//                  [--nqueries 50] [--seed 42]
//
// Vector reuse: when --vectors points at a snapshot whose identity
// matches, per-doc path+content_hash reuse keys hit and only missing
// docs are embedded (the T9 warm-start policy, same as the engine).

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "../include/chunker.h"
#include "../include/embedder.h"
#include "../include/hnsw.h"
#include "../include/indexer.h"
#include "../include/loader.h"
#include "../include/rrf.h"
#include "../include/vector_index.h"
#include "../include/vector_persist.h"

#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

namespace {

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0)
        .count();
}

struct QueryItem {
    int qid = 0;
    std::string tier;  // "A" (base, every rung) or "B<n>" (born rung n)
    std::string text;
    std::vector<std::string> gold;  // doc paths, doc-level labels
};

std::vector<QueryItem> load_queries(const std::string& path) {
    std::vector<QueryItem> out;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        json j = json::parse(line);
        QueryItem q;
        q.qid = j.at("qid");
        q.tier = j.value("tier", "A");
        q.text = j.at("query");
        for (const auto& g : j.at("gold")) q.gold.push_back(g);
        out.push_back(std::move(q));
    }
    return out;
}

// Doc-level view of chunk hits: top-k distinct doc paths in rank order.
std::vector<std::string> top_docs(const std::vector<SearchResult>& hits,
                                  const std::vector<Chunk>& chunks,
                                  const std::vector<Document>& docs,
                                  int k) {
    std::vector<std::string> out;
    for (const SearchResult& r : hits) {
        if (r.chunk_id < 0 ||
            r.chunk_id >= static_cast<int>(chunks.size()))
            continue;
        const std::string& p = docs[chunks[r.chunk_id].document_id].path;
        bool seen = false;
        for (const std::string& q : out)
            if (q == p) { seen = true; break; }
        if (!seen) out.push_back(p);
        if (static_cast<int>(out.size()) >= k) break;
    }
    return out;
}

// Assemble chunk vectors in global chunk-id order. Snapshot docs whose
// path+content_hash match are reused as-is; the rest are embedded in
// one batch. Returns {vectors, reused_docs, embedded_docs}.
struct VectorResult {
    std::vector<std::vector<float>> vectors;
    int reused_docs = 0;
    int embedded_docs = 0;
};

VectorResult gather_vectors(
        const std::vector<Document>& docs,
        const std::vector<Chunk>& chunks,
        const std::string& snapshot_path,
        EmbedProvider& provider) {
    VectorResult res;
    res.vectors.assign(chunks.size(), std::vector<float>());

    std::unordered_map<std::string, std::uint64_t> snap_hash;
    std::unordered_map<std::string,
                       const std::vector<std::vector<float>>*> snap_vecs;
    std::vector<DocVectors> snap;  // must outlive snap_vecs (holds &d.vectors)
    if (!snapshot_path.empty()) {
        VectorIdentity expected{provider.model_id(),
                                provider.embedding_policy(), 1024};
        if (load_vector_snapshot(snapshot_path, expected, snap) ==
            VectorSnapshotStatus::OK) {
            for (const DocVectors& d : snap) {
                snap_hash.emplace(d.path, d.content_hash);
                snap_vecs.emplace(d.path, &d.vectors);
            }
            std::cerr << "[vectors] snapshot reusable docs: "
                      << snap.size() << "\n";
        } else {
            std::cerr << "[vectors] snapshot unusable (" << snapshot_path
                      << "), embedding everything\n";
        }
    }

    // doc id -> its chunk ids in global order
    std::unordered_map<int, std::vector<int>> doc_chunk_ids;
    for (const Chunk& c : chunks) doc_chunk_ids[c.document_id].push_back(c.id);

    std::vector<std::string> pending_texts;
    std::vector<int> pending_chunk_ids;
    for (const Document& doc : docs) {
        const std::vector<int>& cids = doc_chunk_ids[doc.id];
        if (cids.empty()) continue;
        auto it = snap_vecs.find(doc.path);
        const bool reusable = it != snap_vecs.end() &&
            snap_hash[doc.path] == vector_content_hash(doc.content) &&
            it->second->size() == cids.size();
        if (reusable) {
            for (std::size_t i = 0; i < cids.size(); ++i)
                res.vectors[cids[i]] = (*it->second)[i];
            ++res.reused_docs;
        } else {
            for (int cid : cids) {
                pending_texts.push_back(chunks[cid].text);
                pending_chunk_ids.push_back(cid);
            }
            ++res.embedded_docs;
        }
    }
    if (!pending_texts.empty()) {
        std::cerr << "[vectors] embedding " << pending_texts.size()
                  << " chunks from " << res.embedded_docs << " docs\n";
        const auto t0 = Clock::now();
        std::vector<std::vector<float>> fresh =
            provider.embed_documents(pending_texts);
        if (fresh.size() != pending_texts.size())
            throw std::runtime_error("embedder count mismatch");
        for (std::size_t i = 0; i < fresh.size(); ++i)
            res.vectors[pending_chunk_ids[i]] = std::move(fresh[i]);
        std::cerr << "[vectors] embedded in " << ms_since(t0) << " ms\n";
    }
    return res;
}

int run_real(int argc, char** argv) {
    std::string data, vectors, queries, out;
    unsigned seed = 42;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--data") && i + 1 < argc)
            data = argv[++i];
        else if (!std::strcmp(argv[i], "--vectors") && i + 1 < argc)
            vectors = argv[++i];
        else if (!std::strcmp(argv[i], "--queries") && i + 1 < argc)
            queries = argv[++i];
        else if (!std::strcmp(argv[i], "--out") && i + 1 < argc)
            out = argv[++i];
        else if (!std::strcmp(argv[i], "--seed") && i + 1 < argc)
            seed = static_cast<unsigned>(std::stoul(argv[++i]));
    }
    if (data.empty() || queries.empty() || out.empty()) {
        std::cerr << "usage: --data DIR --queries FILE --out FILE "
                     "[--vectors FILE] [--seed N]\n";
        return 2;
    }

    auto provider = make_ollama_provider(
        EmbedderConfig{"", "qwen3-embedding:0.6b", 0});

    std::vector<Document> docs = load_documents(data);
    std::vector<Chunk> chunks = chunk_documents(docs);
    const auto t_idx0 = Clock::now();
    InvertedIndex index = build_index(chunks);
    const double bm25_build_ms = ms_since(t_idx0);

    VectorResult vr = gather_vectors(docs, chunks, vectors, *provider);
    std::cerr << "[dbg] vectors gathered: " << vr.vectors.size()
              << " (reused " << vr.reused_docs << " embedded "
              << vr.embedded_docs << ")\n";

    const auto t_flat0 = Clock::now();
    FlatIndex flat = build_flat_index(vr.vectors);
    const double flat_build_ms = ms_since(t_flat0);
    const auto t_hn0 = Clock::now();
    HnswIndex hnsw = build_hnsw(vr.vectors, seed);
    const double hnsw_build_ms = ms_since(t_hn0);
    std::cerr << "[dbg] flat+hnsw built\n";

    std::ofstream outf(out);
    outf << json{{"type", "meta"}, {"docs", docs.size()},
                 {"chunks", chunks.size()},
                 {"bm25_build_ms", bm25_build_ms},
                 {"flat_build_ms", flat_build_ms},
                 {"hnsw_build_ms", hnsw_build_ms},
                 {"reused_docs", vr.reused_docs},
                 {"embedded_docs", vr.embedded_docs}}.dump() << "\n";

    std::vector<QueryItem> qs = load_queries(queries);
    std::vector<std::vector<float>> qvecs(qs.size());
    std::vector<double> qembed_ms(qs.size(), 0.0);
    for (std::size_t i = 0; i < qs.size(); ++i) {
        const auto t0 = Clock::now();
        qvecs[i] = provider->embed_query(qs[i].text);
        qembed_ms[i] = ms_since(t0);
    }

    for (std::size_t i = 0; i < qs.size(); ++i) {
        const QueryItem& q = qs[i];
        const auto tb = Clock::now();
        std::vector<SearchResult> bm20 = search(index, q.text, 20);
        const double bm25_ms = ms_since(tb);
        const auto tf = Clock::now();
        std::vector<SearchResult> fl20 =
            search_flat(flat, qvecs[i], 20);
        const double flat_ms = ms_since(tf);
        const auto th = Clock::now();
        std::vector<SearchResult> hn20 =
            search_hnsw(hnsw, qvecs[i], 20);
        const double hnsw_ms = ms_since(th);
        const auto tr = Clock::now();
        std::vector<SearchResult> fused = rrf_fuse({bm20, fl20});
        const double rrf_ms = ms_since(tr);

        json row;
        row["type"] = "query";
        row["qid"] = q.qid;
        row["tier"] = q.tier;
        row["routes"]["bm25"] = top_docs(bm20, chunks, docs, 10);
        row["routes"]["flat"] = top_docs(fl20, chunks, docs, 10);
        row["routes"]["hnsw"] = top_docs(hn20, chunks, docs, 10);
        row["routes"]["hybrid"] = top_docs(fused, chunks, docs, 10);
        row["lat_ms"] = {{"bm25", bm25_ms}, {"flat", flat_ms},
                         {"hnsw", hnsw_ms}, {"rrf", rrf_ms},
                         {"embed_query", qembed_ms[i]}};
        outf << row.dump() << "\n";
    }
    std::cerr << "[done] docs=" << docs.size()
              << " chunks=" << chunks.size()
              << " queries=" << qs.size()
              << " reused=" << vr.reused_docs
              << " embedded=" << vr.embedded_docs << "\n";
    return 0;
}

int run_synthetic(int argc, char** argv) {
    long n = 0, dim = 1024, nq = 50;
    std::string out;
    unsigned seed = 42;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--synthetic") && i + 1 < argc)
            n = std::stol(argv[++i]);
        else if (!std::strcmp(argv[i], "--out") && i + 1 < argc)
            out = argv[++i];
        else if (!std::strcmp(argv[i], "--dim") && i + 1 < argc)
            dim = std::stol(argv[++i]);
        else if (!std::strcmp(argv[i], "--nqueries") && i + 1 < argc)
            nq = std::stol(argv[++i]);
        else if (!std::strcmp(argv[i], "--seed") && i + 1 < argc)
            seed = static_cast<unsigned>(std::stoul(argv[++i]));
    }
    if (n <= 0 || out.empty()) {
        std::cerr << "usage: --synthetic N --out FILE [--dim D] "
                     "[--nqueries Q] [--seed S]\n";
        return 2;
    }
    std::mt19937 rng(seed);
    std::normal_distribution<double> gauss(0.0, 1.0);
    auto unit = [&]() {
        std::vector<float> v(dim);
        double norm = 0;
        for (auto& x : v) {
            double g = gauss(rng);
            x = static_cast<float>(g);
            norm += g * g;
        }
        norm = std::sqrt(norm);
        for (auto& x : v) x = static_cast<float>(x / norm);
        return v;
    };
    std::vector<std::vector<float>> base(n);
    for (auto& v : base) v = unit();

    const auto t0 = Clock::now();
    FlatIndex flat = build_flat_index(base);
    const double flat_build = ms_since(t0);
    const auto t1 = Clock::now();
    HnswIndex hnsw = build_hnsw(base, seed);
    const double hnsw_build = ms_since(t1);

    std::vector<std::vector<float>> queries(nq);
    for (auto& q : queries) {
        const std::vector<float>& anchor = base[rng() % base.size()];
        std::vector<float> noise = unit();
        double norm = 0;
        std::vector<float> mix(dim);
        for (long d = 0; d < dim; ++d) {
            mix[d] = 0.9f * anchor[d] + 0.1f * noise[d];
            norm += static_cast<double>(mix[d]) * mix[d];
        }
        norm = std::sqrt(norm);
        for (long d = 0; d < dim; ++d)
            mix[d] = static_cast<float>(mix[d] / norm);
        q = mix;
    }

    double recall_sum = 0, flat_ms = 0, hnsw_ms = 0;
    for (long i = 0; i < nq; ++i) {
        const auto ta = Clock::now();
        std::vector<SearchResult> fr = search_flat(flat, queries[i], 10);
        const auto tb = Clock::now();
        std::vector<SearchResult> hr = search_hnsw(hnsw, queries[i], 10);
        const auto tc = Clock::now();
        flat_ms += ms_since(ta);
        hnsw_ms += ms_since(tb);
        int hit = 0;
        for (const auto& r : hr)
            for (const auto& f : fr)
                if (f.chunk_id == r.chunk_id) { ++hit; break; }
        recall_sum += static_cast<double>(hit) / 10.0;
    }

    std::ofstream outf(out, std::ios::app);
    outf << json{{"type", "synthetic"}, {"n", n}, {"dim", dim},
                 {"queries", nq},
                 {"recall10", recall_sum / nq},
                 {"flat_q_ms", flat_ms / nq},
                 {"hnsw_q_ms", hnsw_ms / nq},
                 {"flat_build_ms", flat_build},
                 {"hnsw_build_ms", hnsw_build}}.dump() << "\n";
    std::cerr << "[synthetic] n=" << n << " recall10=" << recall_sum / nq
              << " flat=" << flat_ms / nq << "ms hnsw=" << hnsw_ms / nq
              << "ms (build " << flat_build << " / " << hnsw_build
              << "ms)\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 3 && !std::strcmp(argv[1], "--synthetic"))
        return run_synthetic(argc, argv);
    return run_real(argc, argv);
}
