// query_worker.h -- the whole query chain, off the UI thread (M4 step 4).
//
// Hard gate from the M4 plan: no worker, no generation. A CPU answer
// from the frozen 4B model takes ~8s and the embed_query HTTP call
// blocks for hundreds of milliseconds too, so the UI thread must never
// run any part of the chain -- it only launches this worker and
// renders what comes back. Both query paths ride it: the retrieval-
// only search (generator == nullptr) and the full RAG ask.
//
// Deliberately NOT a QObject: this GUI avoids moc by design
// (gui/README.md). The caller wraps run_query_chain() in a QThread and
// marshals the two notifications back to the UI thread through
// QMetaObject::invokeMethod functors, which need no Q_OBJECT and no
// metatype registration. All pointers in QueryRequest are borrowed
// from MainWindow and guaranteed alive for the whole run because the
// window refuses corpus mutations while the worker is busy.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "context.h"
#include "embedder.h"
#include "generator.h"
#include "indexer.h"
#include "type.h"
#include "vector_index.h"

struct QueryRequest {
    std::string query;
    const InvertedIndex* index = nullptr;
    const FlatIndex* denseIndex = nullptr;
    const std::vector<Document>* docs = nullptr;
    const std::vector<Chunk>* chunks = nullptr;
    EmbedProvider* embedProvider = nullptr; // embed_query is non-const
    LLMProvider* generator = nullptr;       // nullptr = retrieval only
    bool denseReady = false;
};

struct QueryChatOutcome {
    bool attempted = false;   // generator was present
    bool ok = false;          // chat() returned
    std::string answer;       // final answer text, UTF-8
    std::string error;        // human-readable failure reason
    std::string modelId;
    double seconds = 0.0;
};

struct QueryResult {
    std::vector<SearchResult> bm25;   // lexical route, pre-fusion
    std::vector<SearchResult> dense;  // semantic route, pre-fusion
    bool hybrid = false;              // dense route contributed
    std::string denseError;           // non-empty = dense degraded
    std::vector<SearchResult> ranked; // fused, TOP_K-truncated
    RagPrompt prompt;                 // used_materials = the ledger
    QueryChatOutcome chat;
    double retrievalSeconds = 0.0;
};

using QueryResultPtr = std::shared_ptr<const QueryResult>;

// Runs the whole chain on the calling thread. on_generation_started,
// when set, fires (on the calling thread) right before the blocking
// chat() call so the UI can swap its stage label.
QueryResultPtr run_query_chain(
    const QueryRequest& request,
    const std::function<void()>& on_generation_started);
