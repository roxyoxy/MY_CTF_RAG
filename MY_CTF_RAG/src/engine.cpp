// engine.cpp
// Implements the contract in include/engine.h.
//
// Everything here used to live in the frontends -- twice. The console
// had it in main.cpp; the Qt GUI had a near-copy in mainwindow.cpp and
// query_worker.cpp (the copy that ran the 20-second corpus embedding on
// the UI thread). The contract's three disciplines are what make one
// implementation serve all of them: no printing, no input reading, no
// threads. Every value the old code printed is now returned as data.

#include "engine.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <unordered_map>
#include <utility>

#include "chunker.h"
#include "context.h"
#include "corpus_diff.h"
#include "embedder.h"
#include "generator.h"
#include "loader.h"
#include "rrf.h"
#include "vector_index.h"

namespace {

// Caller-side wiring constants for the hybrid query chain (M3 step 4),
// registered in docs/PARAMS.md. They used to be duplicated in main.cpp
// and gui/query_worker.cpp; this is now their single home.
constexpr int kHybridRouteK = 20;   // candidates each route feeds RRF
constexpr int kEmbedSlice = 32;     // corpus embedding batch size

std::unordered_map<int, float>
score_map(const std::vector<SearchResult>& results) {
    std::unordered_map<int, float> out;
    for (const SearchResult& r : results)
        out[r.chunk_id] = r.score;
    return out;
}

// A route that never saw the chunk reports present == false, which is
// what the console renders as "-" and the GUI as a missing route badge.
RouteScore route_score(const std::unordered_map<int, float>& scores,
                       int chunk_id) {
    const auto it = scores.find(chunk_id);
    if (it == scores.end())
        return RouteScore{};
    return RouteScore{true, it->second};
}

}  // namespace

struct RagEngine::Impl {
    EngineConfig config;
    EngineStatus status;

    std::vector<Document> docs;
    std::vector<Chunk> chunks;
    InvertedIndex index;

    std::unique_ptr<EmbedProvider> provider;
    FlatIndex dense_index;
    bool dense_ready = false;

    std::unique_ptr<LLMProvider> generator;
    bool generation_ready = false;

    // One progress tick. Returns false when the caller asked to stop;
    // every long loop must check the return value. An absent callback
    // means "nobody is watching", which is not a cancellation.
    static bool report(const EngineProgressFn& on_progress,
                       const char* stage, std::size_t done,
                       std::size_t total, std::size_t reused) {
        if (!on_progress)
            return true;
        return on_progress(EngineProgress{stage, done, total, reused});
    }

    // The cheap pre-flight behind EnginePlan. Never chunks -- see the
    // comment on the struct for why that matters.
    EnginePlan analyze() {
        EnginePlan plan;
        plan.documents = docs.size();
        plan.index_state = validate(config.index_snapshot, docs);
        plan.index_needs_rebuild =
            plan.index_state != SnapshotStatus::OK;
        if (config.enable_dense) {
            // Judging the vector snapshot needs the identity, and the
            // provider that answers it costs no network to build.
            provider = make_ollama_provider(
                EmbedderConfig{std::string(), config.dense_model, 0});
            std::vector<DocVectors> ignore;
            plan.vector_state = load_vector_snapshot(
                config.vector_snapshot,
                VectorIdentity{provider->model_id(),
                               provider->embedding_policy(), 0},
                ignore);
            // Missing, unusable, or built by another model: every
            // vector is new. A changed corpus: some of them are.
            plan.vector_needs_work =
                plan.vector_state != VectorSnapshotStatus::OK ||
                plan.index_state == SnapshotStatus::CORPUS_CHANGED;
        }
        return plan;
    }

    // Carry tombstones across a corpus change. An unreadable old
    // snapshot yields no inheritance -- a plain rebuild, the same
    // choice the corrupt-snapshot path makes.
    CorpusChange harvest(const std::string& snapshot) {
        CorpusChange change;
        std::vector<Document> old_docs;
        std::vector<Chunk>    old_chunks;
        InvertedIndex         old_index;
        if (!load(snapshot, old_docs, old_chunks, old_index))
            return change;

        const CorpusDiff diff = diff_corpora(old_docs, docs);
        change.detected = true;
        change.added = diff.added.size();
        change.removed = diff.removed.size();
        change.edited = diff.edited.size();
        change.tombstones_inherited = static_cast<std::size_t>(
            inherit_tombstones(old_docs, docs));
        return change;
    }

    // The BM25 stage: restore the snapshot when it is trustworthy,
    // otherwise rebuild and overwrite it (persist.h keeps the trust
    // policy in the caller -- this is that caller).
    //
    // forced == true rebuilds whether or not the snapshot validates.
    void build_bm25(const std::string& snapshot, bool forced,
                    const EngineProgressFn& on_progress) {
        const SnapshotStatus st = forced ? SnapshotStatus::NOT_FOUND
                                         : validate(snapshot, docs);
        status.index_state = st;

        if (st == SnapshotStatus::OK &&
            load(snapshot, docs, chunks, index)) {
            status.index_restored = true;
        } else {
            status.index_restored = false;
            if (st == SnapshotStatus::CORPUS_CHANGED)
                status.corpus_change = harvest(snapshot);
            chunks = chunk_documents(docs);
            index = build_index(chunks);
            status.index_saved = save(snapshot, docs, chunks, index);
        }

        status.loaded = true;
        status.documents = static_cast<int>(docs.size());
        status.chunks = static_cast<int>(chunks.size());
        status.avg_chunk_length = index.avg_chunk_length;
        // Fires once the stage has RESOLVED, so a frontend can render
        // the startup report at exactly the moment it becomes true.
        report(on_progress, "index", 0, 0, 0);
    }

    // The dense stage. Returns false when the caller cancelled.
    //
    // Throws on provider failure -- open() turns that into a degraded
    // session rather than an error, because BM25 alone still answers.
    bool build_dense(const EngineProgressFn& on_progress) {
        dense_index = FlatIndex{};
        dense_ready = false;
        if (chunks.empty() || !provider)
            return true;
        const auto t_dense = std::chrono::steady_clock::now();

        std::vector<DocVectors> cached;
        const VectorIdentity expected{
            provider->model_id(), provider->embedding_policy(), 0};
        status.vector_state = load_vector_snapshot(
            config.vector_snapshot, expected, cached);

        std::unordered_map<std::string, const DocVectors*> old;
        for (const DocVectors& d : cached)
            old.emplace(d.path, &d);

        // Chunk indices grouped per document (chunker emits chunks in
        // document order, so one pass is enough).
        std::vector<std::vector<std::size_t>> per_doc(docs.size());
        for (std::size_t i = 0; i < chunks.size(); ++i)
            per_doc[static_cast<std::size_t>(chunks[i].document_id)]
                .push_back(i);

        std::vector<DocVectors> records;
        records.reserve(docs.size());
        std::vector<std::string> to_embed;                 // in chunk order
        std::vector<std::pair<std::size_t, std::size_t>> pending;  // (record, n)
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
                for (std::size_t ci :
                     per_doc[static_cast<std::size_t>(doc.id)])
                    to_embed.push_back(chunks[ci].text);
                pending.emplace_back(records.size(), n);
            }
            records.push_back(std::move(rec));
        }

        status.vectors_reused = reused;
        status.vectors_embedded = 0;

        if (!to_embed.empty()) {
            const std::size_t total = to_embed.size();
            if (!report(on_progress, "vectors", 0, total, reused))
                return false;   // cancelled before spending anything

            std::vector<std::vector<float>> embeds;
            embeds.reserve(total);
            for (std::size_t begin = 0; begin < total;
                 begin += static_cast<std::size_t>(kEmbedSlice)) {
                const std::size_t end = std::min(
                    begin + static_cast<std::size_t>(kEmbedSlice), total);
                const std::vector<std::string> slice(
                    to_embed.begin() + static_cast<std::ptrdiff_t>(begin),
                    to_embed.begin() + static_cast<std::ptrdiff_t>(end));
                const std::vector<std::vector<float>> part =
                    provider->embed_documents(slice);
                embeds.insert(embeds.end(), part.begin(), part.end());
                if (!report(on_progress, "vectors", end, total, reused))
                    return false;   // cancelled: nothing is written to disk
            }

            std::size_t k = 0;
            for (const auto& p : pending) {
                DocVectors& rec = records[p.first];
                for (std::size_t j = 0; j < p.second; ++j)
                    rec.vectors.push_back(embeds[k++]);
            }
            status.vectors_embedded = total;
        }

        std::vector<std::vector<float>> all;
        for (const DocVectors& dv : records)
            all.insert(all.end(), dv.vectors.begin(), dv.vectors.end());

        status.vectors_total = all.size();
        dense_index = build_flat_index(all);
        status.vector_saved = vector_save(config.vector_snapshot,
            VectorIdentity{provider->model_id(),
                           provider->embedding_policy(),
                           dense_index.dimension},
            records);
        status.dimension = dense_index.dimension;
        status.dense_model_id = provider->model_id();
        status.dense_policy = provider->embedding_policy();
        status.dense_seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t_dense).count();
        dense_ready = true;
        return true;
    }

    // Chunk position lookup for the two result structures. Out-of-range
    // ids are a registry mismatch, not a crash: the caller skips them.
    bool locate(int chunk_id, std::string& doc_path,
                ChunkLocation& location) const {
        if (chunk_id < 0 ||
            static_cast<std::size_t>(chunk_id) >= chunks.size())
            return false;
        const Chunk& c = chunks[static_cast<std::size_t>(chunk_id)];
        if (c.document_id < 0 ||
            static_cast<std::size_t>(c.document_id) >= docs.size())
            return false;
        const Document& d = docs[static_cast<std::size_t>(c.document_id)];
        doc_path = d.path;
        location = ChunkLocation{c.document_id,
                                 static_cast<std::size_t>(c.begin),
                                 static_cast<std::size_t>(c.end)};
        return true;
    }
};

RagEngine::RagEngine()
    : impl_(new Impl()) {}

RagEngine::~RagEngine() = default;

EngineStatus RagEngine::open(const EngineConfig& config,
                             const EngineProgressFn& on_progress) {
    impl_->config = config;
    impl_->status = EngineStatus{};
    const auto t0 = std::chrono::steady_clock::now();

    // BM25 stage first, and unconditionally: no corpus, no engine. This
    // is the stage that must finish (and save) before anything slower
    // starts, which is what makes cancelling the vector stage safe.
    impl_->docs = load_documents(config.data_dir);
    impl_->status.documents = static_cast<int>(impl_->docs.size());

    // Plan before work: the frontend gets to ask the user first, and a
    // false from the callback stops here having done nothing.
    impl_->status.plan = impl_->analyze();
    if (!Impl::report(on_progress, "plan", 0, 0, 0)) {
        impl_->status.cancelled = true;
        return impl_->status;
    }

    impl_->build_bm25(config.index_snapshot, false, on_progress);

    // Dense stage: the slow one, and the only one that reports real
    // progress. A provider failure degrades the session, it does not
    // fail it.
    if (config.enable_dense && !impl_->chunks.empty()) {
        try {
            // analyze() already built the provider (zero network).
            try {
                if (!impl_->build_dense(on_progress)) {
                    impl_->status.cancelled = true;
                    impl_->dense_ready = false;
                    impl_->status.dense_ready = false;
                }
            } catch (const std::exception& e) {
                impl_->dense_index = FlatIndex{};
                impl_->dense_ready = false;
                impl_->status.dense_ready = false;
                impl_->status.dense_error = e.what();
            }
        } catch (const std::exception& e) {
            impl_->provider.reset();
            impl_->status.dense_error = e.what();
        }
    }
    impl_->status.dense_ready = impl_->dense_ready;

    // Generation stage: construction performs no network I/O
    // (generator.h clause 6); the first chat call is the health check.
    if (config.enable_generation) {
        try {
            impl_->generator = make_ollama_chat_provider(
                GeneratorConfig{std::string(), config.generation_model, 0});
            impl_->generation_ready = true;
        } catch (const std::exception&) {
            impl_->generation_ready = false;
        }
    }
    impl_->status.generation_ready = impl_->generation_ready;
    impl_->status.seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    return impl_->status;
}

QueryOutcome RagEngine::retrieve(const std::string& text,
                                 std::size_t top_k) {
    QueryOutcome out;
    const auto t0 = std::chrono::steady_clock::now();

    const std::vector<SearchResult> bm25 =
        search(impl_->index, text, kHybridRouteK);

    std::vector<SearchResult> dense;
    if (impl_->dense_ready && impl_->provider) {
        try {
            dense = search_flat(impl_->dense_index,
                impl_->provider->embed_query(text), kHybridRouteK);
        } catch (const std::exception& e) {
            // This query drops to BM25-only; the lexical route still
            // answers. The session-level dense state is untouched.
            out.dense_error = e.what();
        }
    }

    out.hybrid = !dense.empty();
    std::vector<SearchResult> ranked =
        out.hybrid ? rrf_fuse({bm25, dense}) : bm25;
    if (top_k == 0)
        top_k = 1;
    if (ranked.size() > top_k)
        ranked.resize(top_k);

    const std::unordered_map<int, float> bm25_scores = score_map(bm25);
    const std::unordered_map<int, float> dense_scores = score_map(dense);
    out.hits.reserve(ranked.size());
    for (const SearchResult& r : ranked) {
        std::string doc_path;
        ChunkLocation location;
        if (!impl_->locate(r.chunk_id, doc_path, location))
            continue;  // registry mismatch: degrade one entry, never crash
        const Chunk& c =
            impl_->chunks[static_cast<std::size_t>(r.chunk_id)];
        QueryHit hit;
        hit.rank = static_cast<int>(out.hits.size()) + 1;
        hit.chunk_id = r.chunk_id;
        hit.doc_path = doc_path;
        hit.location = location;
        hit.text = c.text;
        hit.rrf = r.score;
        hit.bm25 = route_score(bm25_scores, r.chunk_id);
        hit.dense = route_score(dense_scores, r.chunk_id);
        out.hits.push_back(std::move(hit));
    }
    out.retrieval_seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    return out;
}

void RagEngine::generate(const std::string& text, QueryOutcome& out) {
    // No material, no grounded answer -- skipping beats inviting an
    // ungrounded one (contract clause 4).
    if (out.hits.empty())
        return;
    if (!impl_->generation_ready || !impl_->generator)
        return;

    out.generation_attempted = true;
    out.generation_model = impl_->generator->model_id();

    std::vector<RagMaterial> materials;
    materials.reserve(out.hits.size());
    for (const QueryHit& h : out.hits)
        materials.push_back(RagMaterial{h.chunk_id, h.doc_path, h.text});

    const RagPrompt prompt =
        build_rag_prompt(text, materials, CONTEXT_BYTE_BUDGET);

    const auto g0 = std::chrono::steady_clock::now();
    try {
        out.answer = impl_->generator->chat(
            {{ChatRole::SYSTEM, prompt.system_message},
             {ChatRole::USER, prompt.user_message}});
        out.generation_ok = true;
    } catch (const std::exception& e) {
        // Generation is an enhancement, never a dependency.
        out.generation_error = e.what();
    }
    out.generation_seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - g0).count();

    // used_materials IS the citation ledger: [n] in the answer text is
    // the n-th entry here. Positions ride along so a frontend can jump
    // straight to the passage.
    out.citations.reserve(prompt.used_materials.size());
    for (std::size_t i = 0; i < prompt.used_materials.size(); ++i) {
        const RagMaterial& m = prompt.used_materials[i];
        Citation citation;
        citation.n = static_cast<int>(i) + 1;
        citation.chunk_id = m.chunk_id;
        citation.doc_path = m.source;
        ChunkLocation location;
        std::string path;
        if (impl_->locate(m.chunk_id, path, location))
            citation.location = location;
        out.citations.push_back(std::move(citation));
    }
}

std::vector<DocInfo> RagEngine::documents() const {
    std::vector<DocInfo> out;
    out.reserve(impl_->docs.size());
    for (const Document& d : impl_->docs) {
        DocInfo info;
        info.id = d.id;
        info.path = d.path;
        info.bytes = d.content.size();
        info.deleted = d.deleted;
        out.push_back(std::move(info));
    }
    return out;
}

bool RagEngine::document_text(int id, std::string& out) const {
    if (id < 0 || static_cast<std::size_t>(id) >= impl_->docs.size())
        return false;
    out = impl_->docs[static_cast<std::size_t>(id)].content;
    return true;
}

MutationOutcome RagEngine::remove_document(
    int id, const EngineProgressFn& on_progress) {
    MutationOutcome out;

    if (!impl_->status.loaded) {
        out.status = MutationStatus::NOT_LOADED;
        return out;
    }
    if (id < 0 || static_cast<std::size_t>(id) >= impl_->docs.size()) {
        out.status = MutationStatus::NO_SUCH_DOCUMENT;
        return out;
    }
    Document& doc = impl_->docs[static_cast<std::size_t>(id)];
    out.path = doc.path;
    if (doc.deleted) {
        out.status = MutationStatus::ALREADY_DELETED;
        return out;
    }

    // Delete = tombstone + immediate rebuild + save, so the snapshot
    // and the running state never disagree.
    doc.deleted = true;
    impl_->build_bm25(impl_->config.index_snapshot, true, on_progress);

    // Re-chunking shifts chunk ids, so the dense side is rebuilt too --
    // but the survivors hit the vector snapshot by path + hash, so a
    // delete normally costs zero re-embedding and zero network.
    if (impl_->config.enable_dense && impl_->provider &&
        !impl_->chunks.empty()) {
        try {
            if (!impl_->build_dense(on_progress))
                impl_->status.cancelled = true;
        } catch (const std::exception& e) {
            impl_->dense_index = FlatIndex{};
            impl_->dense_ready = false;
            impl_->status.dense_error = e.what();
        }
    }
    impl_->status.dense_ready = impl_->dense_ready;
    impl_->status.documents = static_cast<int>(impl_->docs.size());
    impl_->status.chunks = static_cast<int>(impl_->chunks.size());

    out.status = MutationStatus::OK;
    out.documents = impl_->status.documents;
    out.chunks = impl_->status.chunks;
    out.dense_ready = impl_->dense_ready;
    return out;
}

EngineStatus RagEngine::rebuild(const EngineProgressFn& on_progress) {
    if (!impl_->status.loaded)
        return impl_->status;

    impl_->build_bm25(impl_->config.index_snapshot, true, on_progress);
    impl_->status.documents = static_cast<int>(impl_->docs.size());
    impl_->status.chunks = static_cast<int>(impl_->chunks.size());

    if (impl_->config.enable_dense && impl_->provider) {
        try {
            if (!impl_->build_dense(on_progress))
                impl_->status.cancelled = true;
        } catch (const std::exception& e) {
            impl_->dense_index = FlatIndex{};
            impl_->dense_ready = false;
            impl_->status.dense_error = e.what();
        }
    }
    impl_->status.dense_ready = impl_->dense_ready;
    return impl_->status;
}

const EngineStatus& RagEngine::status() const {
    return impl_->status;
}

const EngineConfig& RagEngine::config() const {
    return impl_->config;
}
