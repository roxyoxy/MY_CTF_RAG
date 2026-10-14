// query_worker.h -- one query, off the UI thread (M4 step 4, M5 rewired).
//
// Hard gate from the M4 plan: no worker, no generation. A CPU answer
// from the frozen 4B model takes seconds and the embed_query HTTP call
// blocks for hundreds of milliseconds, so the UI thread must never run
// any part of the chain -- it only launches this worker and renders
// what comes back.
//
// A thin adapter now: the chain itself lives in RagEngine (engine.h),
// and this file only decides WHEN the two phases run. Retrieval first,
// then a notification so the UI can update, then generation.
//
// Deliberately NOT a QObject: this GUI avoids moc by design
// (gui/README.md). The caller wraps run_query_chain() in a QThread and
// marshals the notifications back to the UI thread through
// QMetaObject::invokeMethod functors, which need no Q_OBJECT and no
// metatype registration.
//
// Threading: the engine is NOT thread-safe by contract. The window owns
// it and refuses corpus mutations while this worker runs, so exactly
// one thread touches it at a time.

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "engine.h"

struct QueryRequest {
    RagEngine* engine = nullptr;   // borrowed; the window outlives this
    std::string query;
    bool withGeneration = false;
};

struct QueryResult {
    QueryOutcome outcome;          // retrieve() filled, then generate()
};

using QueryResultPtr = std::shared_ptr<const QueryResult>;

// Runs both phases on the calling thread. on_generation_started, when
// set, fires (on the calling thread) right before the blocking
// generate() call so the UI can swap its stage label.
QueryResultPtr run_query_chain(
    const QueryRequest& request,
    const std::function<void()>& on_generation_started);
