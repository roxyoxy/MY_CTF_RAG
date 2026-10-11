#include "query_worker.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <utility>

#include "rrf.h"

namespace {
// Caller-side wiring constant for the hybrid query chain (M3 step 4),
// registered in docs/PARAMS.md; mirrored in src/main.cpp. This file is
// the GUI-side home of the query-chain policy now that the chain moved
// off the UI thread (M4 step 4).
constexpr int kHybridRouteK = 20;
}

QueryResultPtr run_query_chain(
    const QueryRequest& state,
    const std::function<void()>& on_generation_started) {
    auto result = std::make_shared<QueryResult>();
    const auto t0 = std::chrono::steady_clock::now();

    result->bm25 = search(*state.index, state.query, kHybridRouteK);

    if (state.denseReady && state.embedProvider != nullptr &&
        state.denseIndex != nullptr) {
        try {
            result->dense = search_flat(
                *state.denseIndex,
                state.embedProvider->embed_query(state.query),
                kHybridRouteK);
        } catch (const std::exception& e) {
            // Same degradation policy as the console: the dense route
            // drops for this query, the lexical route still answers.
            result->denseError = e.what();
        }
    }

    result->hybrid = !result->dense.empty();
    result->ranked = result->hybrid
        ? rrf_fuse({result->bm25, result->dense})
        : result->bm25;
    if (result->ranked.size() > static_cast<size_t>(TOP_K)) {
        result->ranked.resize(static_cast<size_t>(TOP_K));
    }

    // Mirror of the console Stage 4: materials come straight from the
    // fused top results. Bounds are checked defensively -- a registry
    // mismatch must degrade one entry, not crash the worker thread.
    std::vector<RagMaterial> materials;
    materials.reserve(result->ranked.size());
    for (const SearchResult& r : result->ranked) {
        if (r.chunk_id < 0 ||
            static_cast<size_t>(r.chunk_id) >= state.chunks->size()) {
            continue;
        }
        const Chunk& chunk = (*state.chunks)[static_cast<size_t>(r.chunk_id)];
        if (chunk.document_id < 0 ||
            static_cast<size_t>(chunk.document_id) >= state.docs->size()) {
            continue;
        }
        const Document& doc =
            (*state.docs)[static_cast<size_t>(chunk.document_id)];
        materials.push_back(RagMaterial{r.chunk_id, doc.path, chunk.text});
    }

    result->prompt =
        build_rag_prompt(state.query, materials, CONTEXT_BYTE_BUDGET);
    result->retrievalSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();

    if (state.generator != nullptr) {
        result->chat.attempted = true;
        if (on_generation_started) {
            on_generation_started();
        }
        const auto g0 = std::chrono::steady_clock::now();
        try {
            result->chat.answer = state.generator->chat(
                {{ChatRole::SYSTEM, result->prompt.system_message},
                 {ChatRole::USER, result->prompt.user_message}});
            result->chat.ok = true;
            result->chat.modelId = state.generator->model_id();
        } catch (const std::exception& e) {
            result->chat.error = e.what();
        }
        result->chat.seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - g0).count();
    }

    // Drop the caller's write access: the UI only ever renders this.
    return QueryResultPtr(std::move(result));
}
