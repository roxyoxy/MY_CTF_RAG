// engine.h -- the pipeline engine (contract #16).
//
// Role: hold the whole retrieval pipeline's state and the CALLER-SIDE
// POLICY that used to be duplicated in every frontend -- snapshot trust
// decisions, corpus diff + tombstone harvesting, the hybrid query
// chain, per-document vector reuse, and the degradation rules. One
// implementation, three thin shells (console / Qt GUI / web service).
//
// Three disciplines. A frontend that wants different behavior wraps
// this engine; it does not fork it:
//   1. Never prints. Every outcome is returned as data; presentation
//      belongs to the frontend.
//   2. Never reads input. Command parsing and prompt loops stay in the
//      frontend.
//   3. Never spawns threads. Scheduling belongs to the frontend, so the
//      engine stays synchronous and directly unit-testable.
//
// Threading: NOT thread-safe. One thread at a time, by contract -- the
// caller must guarantee that no other thread touches the engine while
// a call is in flight. Concurrency correctness lives in ONE place (the
// caller's scheduling layer) instead of being spread through locks.

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// IWYU: only what these declarations need -- the two status enums and
// the registered TOP_K default. The provider and generation headers are
// the .cpp's business; including them here would leak them to every
// consumer.
#include "indexer.h"          // TOP_K (the default for query())
#include "persist.h"          // SnapshotStatus
#include "vector_persist.h"   // VectorSnapshotStatus

// Startup configuration. Every value is a runtime knob; nothing here is
// a compile-time constant, so the same binary serves a default install
// and an experiment.
struct EngineConfig {
    std::string data_dir = "data";
    std::string index_snapshot = "index.bin";
    std::string vector_snapshot = "vector.bin";
    std::string dense_model = "qwen3-embedding:0.6b";
    std::string generation_model = "qwen3:4b-instruct";
    bool enable_dense = true;
    bool enable_generation = true;
};

// Progress report. `stage` is a stable English identifier the frontend
// maps to its own wording; the counts describe the CURRENT stage only.
struct EngineProgress {
    std::string stage;          // "index" | "vectors" | "generation"
    std::size_t done = 0;       // items finished in this stage
    std::size_t total = 0;      // items in this stage (0 = indeterminate)
    std::size_t reused = 0;     // items served from a snapshot
};

// The callback returns false to request cancellation. Because it runs
// on the engine's calling thread, a frontend can implement "cancel" as
// a plain flag read inside its own callback (the Qt progress dialog
// works exactly this way) with no cross-thread signalling.
using EngineProgressFn = std::function<bool(const EngineProgress&)>;

// What changed between the previous snapshot's corpus and the current
// one. All-zero with detected == false means "no snapshot to compare".
struct CorpusChange {
    bool detected = false;
    std::size_t added = 0;
    std::size_t removed = 0;
    std::size_t edited = 0;
    std::size_t tombstones_inherited = 0;
};

// Outcome of open() / rebuild(): everything a frontend needs to render
// a startup report, including WHY each side is in the state it is in.
struct EngineStatus {
    bool loaded = false;              // open() has completed at least once
    int documents = 0;
    int chunks = 0;

    // BM25 side
    SnapshotStatus index_state = SnapshotStatus::NOT_FOUND;
    bool index_restored = false;      // true = snapshot hit, no rebuild
    bool index_saved = true;          // false = save failed; a lost cache only
    CorpusChange corpus_change;

    // Dense side
    bool dense_ready = false;
    VectorSnapshotStatus vector_state = VectorSnapshotStatus::NOT_FOUND;
    std::string dense_error;          // non-empty = the dense stage failed
    std::size_t vectors_reused = 0;
    std::size_t vectors_embedded = 0;
    std::size_t vectors_total = 0;    // reused + embedded
    int dimension = 0;
    std::string dense_model_id;
    std::string dense_policy;
    bool vector_saved = true;         // false = save failed; a lost cache only

    // Generation side
    bool generation_ready = false;

    bool cancelled = false;           // the progress callback asked to stop
    double dense_seconds = 0.0;       // the dense stage alone
    double seconds = 0.0;             // the whole open() call
};

// One document as the frontends list it.
struct DocInfo {
    int id = 0;
    std::string path;
    std::size_t bytes = 0;
    bool deleted = false;
};

// Where a chunk lives inside its document. The engine keeps the chunk
// table; frontends only ever need the POSITION -- this is what lets a
// web page or the Qt GUI jump to and highlight a source passage without
// owning a copy of the pipeline state.
struct ChunkLocation {
    int document_id = 0;          // Document::id
    std::size_t begin = 0;        // byte offset into Document::content
    std::size_t end = 0;          // one past the last byte
};

// Per-route scores for one hit, pre-fusion. A route that did not recall
// the chunk reports present == false (the console prints "-").
struct RouteScore {
    bool present = false;
    float score = 0.0f;
};

struct QueryHit {
    int rank = 0;                 // 1-based, after fusion
    int chunk_id = 0;
    std::string doc_path;         // Document::path
    ChunkLocation location;
    std::string text;
    float rrf = 0.0f;
    RouteScore bm25;
    RouteScore dense;
};

// The citation ledger entry: [n] in the answer text maps to this.
struct Citation {
    int n = 0;                    // 1-based, matches [n] in the answer
    int chunk_id = 0;
    std::string doc_path;         // Document::path -- shown as the source
    ChunkLocation location;
};

struct QueryOutcome {
    // Retrieval
    std::vector<QueryHit> hits;   // top_k, fused when the dense side ran
    bool hybrid = false;          // true = RRF over two routes
    std::string dense_error;      // non-empty = this query degraded to BM25-only
    double retrieval_seconds = 0.0;

    // Generation (only when requested and available)
    bool generation_attempted = false;
    bool generation_ok = false;
    std::string answer;
    std::string generation_error;
    std::string generation_model;
    double generation_seconds = 0.0;
    std::vector<Citation> citations;
};

// Why a corpus mutation did or did not happen. A code, not a sentence:
// the frontend owns the wording (the console says English, the GUI says
// Chinese, and neither belongs baked into the engine).
enum class MutationStatus {
    OK,
    NOT_LOADED,          // open() has not run yet
    NO_SUCH_DOCUMENT,    // id outside [0, documents())
    ALREADY_DELETED,     // idempotent: the tombstone is already set
};

// Outcome of a corpus mutation.
struct MutationOutcome {
    MutationStatus status = MutationStatus::NOT_LOADED;
    std::string path;             // the document acted on, when known
    int documents = 0;
    int chunks = 0;               // chunks remaining after the rebuild
    bool dense_ready = false;
};

class RagEngine {
public:
    RagEngine();
    ~RagEngine();

    RagEngine(const RagEngine&) = delete;
    RagEngine& operator=(const RagEngine&) = delete;

    // Load the corpus and bring up all three sides. Synchronous.
    //
    // 1. Fast path: when both snapshots validate, this returns in
    //    milliseconds with zero network (dense vectors are reused).
    //    The callback fires once when the INDEX stage resolves
    //    (restored or rebuilt), so a frontend can render its startup
    //    report in order without polling, and once per embedding
    //    batch during the vector stage.
    // 2. Slow path: the vector stage embeds the chunks that are missing
    //    from the snapshot, in batches, reporting progress per batch.
    //    The callback's return value is honoured between batches.
    // 3. Every side degrades independently: a BM25 failure throws
    //    (there is no usable retrieval without it); a dense or
    //    generation failure is recorded in the status and leaves the
    //    engine usable.
    // 4. Cancellation is not an error. The engine stops at the next
    //    batch boundary, keeps what already succeeded, and reports
    //    cancelled == true.
    // 5. Stage order is part of this contract: the BM25 stage completes
    //    and saves index.bin BEFORE the vector stage starts. Cancelling
    //    the vector stage therefore always leaves a fully usable
    //    lexical engine. A frontend may keep the session alive with
    //    degraded (BM25-only) retrieval instead of exiting, and owes
    //    the user a visible "dense not enabled" state when it does.
    EngineStatus open(const EngineConfig& config,
        const EngineProgressFn& on_progress = nullptr);

    // Phase 1 of the query chain: BM25 + dense routes, RRF fusion,
    // top_k. Fast -- at most one embed_query round trip.
    //
    // 1. A dense failure degrades THIS call to BM25-only and records
    //    dense_error; it does not throw.
    // 2. hits is always the ranked list the frontend should show.
    //    top_k defaults to the registered TOP_K; a frontend may ask
    //    for fewer or more (the web service exposes it), and the
    //    context builder -- not this call -- is what bounds the
    //    generation budget.
    QueryOutcome retrieve(const std::string& text,
        std::size_t top_k = static_cast<std::size_t>(TOP_K));

    // Phase 2: ground an answer in an outcome from retrieve(). Slow
    // -- seconds of local inference.
    //
    // 3. Retrieval and generation are two calls ON PURPOSE. Both
    //    frontends show the hits first and the answer after; a
    //    combined call would make them wait out the model before
    //    showing anything (which is exactly what the console used
    //    to do wrong).
    // 4. Fills generation_attempted / generation_ok /
    //    generation_error / answer / citations. Generation is
    //    skipped -- attempted stays false -- when hits is empty
    //    (no material to ground an answer on) or when generation
    //    is unavailable.
    // 5. A generation failure records generation_error; it does not
    //    throw. Generation is an enhancement, never a dependency.
    void generate(const std::string& text, QueryOutcome& outcome);

    // Corpus views.
    std::vector<DocInfo> documents() const;
    bool document_text(int id, std::string& out) const;

    // Mark a document deleted, then rebuild both derived sides. The
    // surviving documents hit the vector snapshot, so a delete normally
    // costs zero re-embedding.
    MutationOutcome remove_document(int id,
        const EngineProgressFn& on_progress = nullptr);

    // Rebuild the derived sides (same progress and cancellation rules
    // as open()).
    //
    // 1. Each side keeps its own established incremental semantics
    //    rather than inventing a new one: the BM25 side is always
    //    rebuilt in full (M2-3 ruled true incremental out -- packed
    //    chunk ids make a mid-insert renumber cascade cost the same as
    //    a rebuild), while the vector side is per-document and
    //    re-embeds only the documents whose path + content hash +
    //    chunk count changed.
    EngineStatus rebuild(const EngineProgressFn& on_progress = nullptr);

    // Current state without doing any work.
    const EngineStatus& status() const;
    const EngineConfig& config() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
