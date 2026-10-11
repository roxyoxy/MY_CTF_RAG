// main.cpp
// Assembly line: load, chunk, index, then answer queries in a loop.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#ifdef _WIN32
#define NOMINMAX  // keep windows.h min/max macros out of std::min
#include <windows.h>
#endif

#include "type.h"
#include "loader.h"
#include "corpus_diff.h"
#include "chunker.h"
#include "indexer.h"
#include "persist.h"
#include "embedder.h"
#include "vector_index.h"
#include "vector_persist.h"
#include "rrf.h"

// Caller-side wiring constants for the hybrid query chain (M3 step 4),
// registered in docs/PARAMS.md.
constexpr int HYBRID_ROUTE_K = 20;  // candidates each route feeds RRF
constexpr int EMBED_SLICE = 32;     // corpus embedding batch size
// Dense model decided by the A/B experiment (M3 step 2, 2026-10-11):
// qwen3-embedding:0.6b beat bge-m3 on every metric (hit@10 96% vs 83%,
// MRR 0.777 vs 0.697, faster embedding); see docs/PARAMS.md.
constexpr const char* DENSE_MODEL = "qwen3-embedding:0.6b";

// Human-readable name for a snapshot validation failure. The console
// app only prints it; the caller action is always the same (rebuild).
static const char* status_name(SnapshotStatus st) {
    switch (st) {
    case SnapshotStatus::OK:             return "ok";
    case SnapshotStatus::NOT_FOUND:      return "not found (first run)";
    case SnapshotStatus::IO_ERROR:       return "io error";
    case SnapshotStatus::BAD_MAGIC:      return "bad magic";
    case SnapshotStatus::BAD_FORMAT:     return "corrupt";
    case SnapshotStatus::BAD_PIPELINE:   return "pipeline version mismatch";
    case SnapshotStatus::BAD_PARAMS:     return "chunk params mismatch";
    case SnapshotStatus::CORPUS_CHANGED: return "corpus changed";
    }
    return "unknown";
}

// M2-3: when the corpus changed, the old snapshot still remembers
// which documents were deleted. Load it, inherit the tombstones into
// the probe corpus, and report what changed. A snapshot that fails
// to load yields no inheritance -- plain rebuild.
static void harvest_tombstones(const std::string& snapshot,
                               std::vector<Document>& docs) {
    std::vector<Document> old_docs;
    std::vector<Chunk>    old_chunks;
    InvertedIndex         old_index;
    if (!load(snapshot, old_docs, old_chunks, old_index))
        return;

    const CorpusDiff diff = diff_corpora(old_docs, docs);
    const int inherited = inherit_tombstones(old_docs, docs);
    std::cout << "corpus changed: " << diff.added.size() << " added, "
        << diff.removed.size() << " removed, " << diff.edited.size()
        << " edited";
    if (inherited > 0)
        std::cout << ", " << inherited << " tombstone(s) inherited";
    std::cout << "\n";
}

// Human-readable name for a vector snapshot status. The console only
// prints it; the caller action is always the same (fall back to
// embedding through the provider).
static const char* vstatus_name(VectorSnapshotStatus st) {
    switch (st) {
    case VectorSnapshotStatus::OK:         return "ok";
    case VectorSnapshotStatus::NOT_FOUND:  return "not found (first run)";
    case VectorSnapshotStatus::IO_ERROR:   return "io error";
    case VectorSnapshotStatus::BAD_MAGIC:  return "bad magic";
    case VectorSnapshotStatus::BAD_FORMAT: return "corrupt";
    case VectorSnapshotStatus::BAD_MODEL:  return "built by a different model";
    }
    return "unknown";
}

// Chunk-order vector table assembled from the per-document records
// (chunker emits chunks in document order, so concatenation restores
// chunk ids 0..n-1).
static std::vector<std::vector<float>>
flatten_doc_vectors(const std::vector<DocVectors>& docs) {
    std::vector<std::vector<float>> all;
    for (const DocVectors& dv : docs)
        all.insert(all.end(), dv.vectors.begin(), dv.vectors.end());
    return all;
}

// M3 step 5 (T9): per-document vector table with snapshot reuse.
// A document's vectors are reused when path + content hash + chunk
// count all match the snapshot -- the chunker is deterministic, so an
// unchanged document always re-cuts into the same chunks. Everything
// else re-embeds through the provider, sliced for progress
// (EMBED_SLICE). Reuse policy lives here in the caller, per the
// vector_persist contract (facts vs policy, the corpus_diff rule).
static std::vector<DocVectors>
build_doc_vectors(EmbedProvider& provider,
                  const std::vector<Document>& docs,
                  const std::vector<Chunk>& chunks,
                  const std::string& snapshot,
                  const char* when) {
    std::vector<DocVectors> cached;
    const VectorIdentity expected{provider.model_id(),
                                  provider.embedding_policy(), 0};
    const VectorSnapshotStatus st =
        load_vector_snapshot(snapshot, expected, cached);
    if (st == VectorSnapshotStatus::BAD_MODEL)
        std::cout << "vector snapshot " << vstatus_name(st)
                  << ", re-embedding everything.\n";
    else if (st != VectorSnapshotStatus::OK &&
             st != VectorSnapshotStatus::NOT_FOUND)
        std::cout << "vector snapshot unusable ("
                  << vstatus_name(st) << "), re-embedding.\n";

    std::unordered_map<std::string, const DocVectors*> old;
    for (const DocVectors& d : cached)
        old.emplace(d.path, &d);

    // chunk indices grouped per document
    std::vector<std::vector<std::size_t>> per_doc(docs.size());
    for (std::size_t i = 0; i < chunks.size(); ++i)
        per_doc[static_cast<std::size_t>(
            chunks[i].document_id)].push_back(i);

    std::vector<DocVectors> out;
    out.reserve(docs.size());
    std::vector<std::string> to_embed;  // texts in chunk order
    std::vector<std::pair<std::size_t, std::size_t>> pending;  // (out, n)
    std::size_t reused = 0;
    for (const Document& doc : docs) {
        if (doc.deleted)
            continue;  // tombstones never reach the vector side
        DocVectors rec;
        rec.path = doc.path;
        rec.content_hash = vector_content_hash(doc.content);
        const std::size_t n =
            per_doc[static_cast<std::size_t>(doc.id)].size();
        const auto it = old.find(doc.path);
        if (it != old.end() &&
            it->second->content_hash == rec.content_hash &&
            it->second->vectors.size() == n) {
            rec.vectors = it->second->vectors;
            reused += n;
        } else {
            for (std::size_t ci : per_doc[static_cast<std::size_t>(doc.id)])
                to_embed.push_back(chunks[ci].text);
            pending.emplace_back(out.size(), n);
        }
        out.push_back(std::move(rec));
    }

    if (!to_embed.empty()) {
        std::cout << "embedding " << to_embed.size() << " chunk(s) ("
                  << when << ", " << provider.model_id()
                  << " via local Ollama)...\n";
        std::vector<std::vector<float>> embeds;
        embeds.reserve(to_embed.size());
        const std::size_t total = to_embed.size();
        for (std::size_t begin = 0; begin < total; begin += EMBED_SLICE) {
            const std::size_t end = std::min(begin + EMBED_SLICE, total);
            const std::vector<std::string> slice(
                to_embed.begin() + static_cast<std::ptrdiff_t>(begin),
                to_embed.begin() + static_cast<std::ptrdiff_t>(end));
            const std::vector<std::vector<float>> part =
                provider.embed_documents(slice);
            embeds.insert(embeds.end(), part.begin(), part.end());
            std::cout << "  embedding chunks " << end << "/" << total
                      << std::flush << "\r";
        }
        std::cout << "\n";
        std::size_t k = 0;
        for (const auto& p : pending) {
            DocVectors& rec = out[p.first];
            for (std::size_t j = 0; j < p.second; ++j)
                rec.vectors.push_back(embeds[k++]);
        }
    }
    std::cout << "dense vectors: " << reused << " reused from snapshot, "
              << to_embed.size() << " embedded (" << when << ")\n";
    return out;
}

static std::unordered_map<int, float>
score_map(const std::vector<SearchResult>& results) {
    std::unordered_map<int, float> out;
    for (const SearchResult& r : results)
        out[r.chunk_id] = r.score;
    return out;
}

// "-" for a route that did not recall the chunk: the fused printout
// then shows at a glance which route contributed what.
static std::string score_or_dash(const std::unordered_map<int, float>& m,
                                 int chunk_id) {
    const auto it = m.find(chunk_id);
    if (it == m.end())
        return "-";
    std::ostringstream os;
    os << it->second;
    return os.str();
}

int main() {
#ifdef _WIN32
    // Switch the Windows console to UTF-8 so Chinese text shows correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<Document> docs;
    std::vector<Chunk>    chunks;
    InvertedIndex         index;
    const std::string snapshot = "index.bin";

    // Stage 1: build the index once. The snapshot is a cache: restore
    // it when valid, otherwise rebuild and overwrite it (persist.h
    // keeps the trust policy here, in the caller).
    try {
        docs = load_documents("data");

        const SnapshotStatus st = validate(snapshot, docs);
        bool restored = false;
        if (st == SnapshotStatus::OK)
            restored = load(snapshot, docs, chunks, index);

        if (restored) {
            std::cout << "Index restored from snapshot " << snapshot << ".\n";
        } else {
            if (st == SnapshotStatus::OK)
                std::cout << "Snapshot load failed, rebuilding.\n";
            else {
                std::cout << "Snapshot unusable ("
                    << status_name(st) << "), rebuilding.\n";
                if (st == SnapshotStatus::CORPUS_CHANGED)
                    harvest_tombstones(snapshot, docs);
            }
            chunks = chunk_documents(docs);
            index = build_index(chunks);
            if (!save(snapshot, docs, chunks, index))
                std::cerr << "Warning: snapshot save failed; "
                    "the in-memory index is still usable.\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Indexed " << docs.size()
        << " documents, " << chunks.size()
        << " chunks.\n";

    // M3 steps 4 + 5 (T9): dense route with vector.bin caching.
    // An unchanged corpus comes up with zero network (the snapshot
    // loads before any provider call); changed documents re-embed; a
    // snapshot from a different model reports BAD_MODEL and the whole
    // corpus re-embeds. Provider construction performs no network I/O
    // (embedder.h clause 6); the first real embed call is the health
    // check. Failure degrades this session to BM25-only.
    std::unique_ptr<EmbedProvider> provider =
        make_ollama_provider(EmbedderConfig{"", DENSE_MODEL, 0});
    FlatIndex dense_index;
    bool dense_ready = false;
    const std::string vector_snapshot = "vector.bin";
    if (!chunks.empty()) {
        const auto t0 = std::chrono::steady_clock::now();
        try {
            const std::vector<DocVectors> doc_vectors =
                build_doc_vectors(*provider, docs, chunks,
                                  vector_snapshot, "startup");
            dense_index = build_flat_index(flatten_doc_vectors(doc_vectors));
            if (!vector_save(vector_snapshot,
                    VectorIdentity{provider->model_id(),
                                   provider->embedding_policy(),
                                   dense_index.dimension},
                    doc_vectors))
                std::cerr << "Warning: vector snapshot save failed.\n";
            const double secs = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - t0).count();
            dense_ready = true;
            std::cout << "dense route ready: "
                      << dense_index.vectors.size() << " vectors, dim "
                      << dense_index.dimension << " (" << secs << "s)\n";
        } catch (const std::exception& e) {
            dense_index = FlatIndex{};
            dense_ready = false;
            std::cerr << "Warning: dense route unavailable ("
                      << e.what()
                      << "); queries run BM25-only this session.\n";
        }
    }

    std::cout << "commands: list, del <id>  (anything else is a query)\n";

    // Stage 2: query loop. "list" and "del <id>" are reserved words;
    // every other line is a search query.
    std::string query;
    while (true) {
        std::cout << "query> ";
        if (!std::getline(std::cin, query)) break;
        if (query.empty()) continue;

        if (query == "list") {
            for (size_t i = 0; i < docs.size(); ++i) {
                std::cout << docs[i].id
                    << (docs[i].deleted ? "  [deleted]  " : "  ")
                    << docs[i].content.size() << " bytes  "
                    << docs[i].path << "\n";
            }
            continue;
        }

        if (query.rfind("del ", 0) == 0) {
            const std::string arg = query.substr(4);
            bool digits = !arg.empty();
            for (size_t k = 0; k < arg.size(); ++k)
                if (arg[k] < '0' || arg[k] > '9') digits = false;
            if (!digits) {
                std::cout << "usage: del <doc id>\n";
                continue;
            }
            int id = 0;
            for (size_t k = 0; k < arg.size(); ++k)
                id = id * 10 + (arg[k] - '0');
            if (id >= static_cast<int>(docs.size())) {
                std::cout << "no document with id " << id << "\n";
                continue;
            }
            if (docs[id].deleted) {
                std::cout << docs[id].path << " is already deleted\n";
                continue;
            }
            // Delete = tombstone + immediate rebuild + save, so the
            // snapshot and the running state never disagree.
            docs[id].deleted = true;
            chunks = chunk_documents(docs);
            index = build_index(chunks);
            if (!save(snapshot, docs, chunks, index))
                std::cerr << "Warning: snapshot save failed; "
                    "the in-memory index is still updated.\n";
            std::cout << "deleted " << docs[id].path
                << " (" << chunks.size() << " chunks remain)\n";
            // Re-chunking shifts chunk ids, so the dense side must be
            // rebuilt too -- but the surviving documents hit the
            // snapshot (path + hash match), so a delete normally
            // costs zero re-embedding.
            try {
                const std::vector<DocVectors> doc_vectors =
                    build_doc_vectors(*provider, docs, chunks,
                                      vector_snapshot, "after delete");
                dense_index =
                    build_flat_index(flatten_doc_vectors(doc_vectors));
                if (!vector_save(vector_snapshot,
                        VectorIdentity{provider->model_id(),
                                       provider->embedding_policy(),
                                       dense_index.dimension},
                        doc_vectors))
                    std::cerr << "Warning: vector snapshot save failed.\n";
                dense_ready = true;
                std::cout << "dense route rebuilt: "
                          << dense_index.vectors.size() << " vectors\n";
            } catch (const std::exception& e) {
                dense_index = FlatIndex{};
                dense_ready = false;
                std::cerr << "Warning: dense route unavailable ("
                          << e.what() << ").\n";
            }
            continue;
        }

        // Stage 3: search and print. Two routes while the dense side is
        // alive -- BM25 (lexical) + vector (semantic) -- fused by RRF;
        // BM25 alone otherwise, printed with its own raw scores.
        std::vector<SearchResult> bm25 =
            search(index, query, HYBRID_ROUTE_K);
        std::vector<SearchResult> dense;
        if (dense_ready) {
            try {
                dense = search_flat(dense_index,
                                    provider->embed_query(query),
                                    HYBRID_ROUTE_K);
            } catch (const std::exception& e) {
                std::cerr << "Warning: dense route failed ("
                          << e.what() << "); BM25-only for this query.\n";
            }
        }

        if (!dense.empty()) {
            const std::unordered_map<int, float> bm25_score = score_map(bm25);
            const std::unordered_map<int, float> dense_score = score_map(dense);
            const std::vector<SearchResult> fused = rrf_fuse({bm25, dense});

            if (fused.empty()) {
                std::cout << "(no results)\n";
                continue;
            }
            const std::size_t shown =
                std::min(fused.size(), static_cast<std::size_t>(TOP_K));
            for (std::size_t i = 0; i < shown; ++i) {
                const SearchResult& r = fused[i];
                const Chunk& c = chunks[r.chunk_id];
                const Document& d = docs[c.document_id];

                std::cout << "[" << (i + 1) << "] "
                    << "rrf=" << r.score << "  "
                    << "bm25=" << score_or_dash(bm25_score, r.chunk_id)
                    << "  "
                    << "vec=" << score_or_dash(dense_score, r.chunk_id)
                    << "  "
                    << "doc=" << d.path << "\n"
                    << "    " << c.text << "\n";
            }
        } else {
            if (bm25.empty()) {
                std::cout << "(no results)\n";
                continue;
            }
            const std::size_t shown =
                std::min(bm25.size(), static_cast<std::size_t>(TOP_K));
            for (std::size_t i = 0; i < shown; ++i) {
                const SearchResult& r = bm25[i];
                const Chunk& c = chunks[r.chunk_id];
                const Document& d = docs[c.document_id];

                std::cout << "[" << (i + 1) << "] "
                    << "score=" << r.score << "  "
                    << "doc=" << d.path << "\n"
                    << "    " << c.text << "\n";
            }
        }
    }

    return 0;
}