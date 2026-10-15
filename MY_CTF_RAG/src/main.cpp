// main.cpp
// Console front end.
//
// All retrieval and generation orchestration now lives in RagEngine
// (engine.h). What is left here is what only a console owns: reading
// the command line, choosing what to print, and naming the snapshot
// states in English. The engine's three disciplines -- no printing, no
// input, no threads -- are exactly why this is a shell now instead of
// the assembly line it used to be.
//
// The output is deliberately byte-identical to the pre-refactor
// console: that is what proves the move changed structure and not
// behavior. Everything printed below that the engine no longer prints
// itself is reconstructed from EngineStatus / QueryOutcome.

#include <cstddef>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX  // keep windows.h min/max macros out of std::min
#include <windows.h>
#endif

#include "engine.h"

namespace {

// Console wording for a snapshot state. The engine returns the enum;
// choosing words is presentation, so each frontend keeps its own table.
const char* status_name(SnapshotStatus st) {
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

// A route score as the console prints it: the ostream default float
// formatting ("8.82"), and "-" for a route that never saw the chunk.
std::string score_or_dash(const RouteScore& s) {
    if (!s.present)
        return "-";
    std::ostringstream os;
    os << s.score;
    return os.str();
}

// The startup report. It is printed in reaction to the engine's
// "index" tick rather than after open() returns, because the embedding
// progress lines have to come after it -- the same interleaving the old
// inline Stage 1 produced.
void print_startup_report(const EngineStatus& s,
                          const std::string& snapshot) {
    if (s.index_restored) {
        std::cout << "Index restored from snapshot " << snapshot << ".\n";
    } else if (s.index_state == SnapshotStatus::OK) {
        std::cout << "Snapshot load failed, rebuilding.\n";
    } else {
        std::cout << "Snapshot unusable ("
            << status_name(s.index_state) << "), rebuilding.\n";
        if (s.corpus_change.detected) {
            std::cout << "corpus changed: " << s.corpus_change.added
                << " added, " << s.corpus_change.removed << " removed, "
                << s.corpus_change.edited << " edited";
            if (s.corpus_change.tombstones_inherited > 0)
                std::cout << ", " << s.corpus_change.tombstones_inherited
                << " tombstone(s) inherited";
            std::cout << "\n";
        }
    }
    std::cout << "Indexed " << s.documents << " documents, "
        << s.chunks << " chunks.\n";
}

// One embedding progress line, shared by startup and a delete rebuild.
void print_embed_progress(const EngineProgress& p,
                          const std::string& model,
                          const char* when) {
    if (p.done == 0) {
        std::cout << "embedding " << p.total << " chunk(s) (" << when
            << ", " << model << " via local Ollama)...\n";
    } else {
        std::cout << "  embedding chunks " << p.done << "/" << p.total
            << std::flush << "\r";
    }
}

}  // namespace

int main() {
#ifdef _WIN32
    // Switch the Windows console to UTF-8 so Chinese text shows correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    RagEngine engine;
    const EngineConfig config;   // data/, index.bin, vector.bin, both models

    // Stage 1 + dense + generation, all inside open(). The callback
    // reproduces the old output order exactly: the startup report when
    // the index stage resolves, then one line per embedding batch.
    EngineStatus status;
    bool progress_line_open = false;
    try {
        status = engine.open(config,
            [&engine, &progress_line_open](const EngineProgress& p) {
                if (p.stage == "index") {
                    print_startup_report(engine.status(),
                        engine.config().index_snapshot);
                } else if (p.stage == "vectors") {
                    print_embed_progress(p, engine.config().dense_model,
                        "startup");
                    progress_line_open = (p.done != 0);
                }
                return true;
            });
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (progress_line_open)
        std::cout << "\n";   // close the "\r" progress line
    if (status.dense_ready) {
        std::cout << "dense vectors: " << status.vectors_reused
            << " reused from snapshot, " << status.vectors_embedded
            << " embedded (startup)\n";
        std::cout << "dense route ready: " << status.vectors_total
            << " vectors, dim " << status.dimension
            << " (" << status.dense_seconds << "s)\n";
    } else if (status.cancelled) {
        std::cout << "dense route cancelled; queries run BM25-only "
            "this session.\n";
    } else if (!status.dense_error.empty()) {
        std::cerr << "Warning: dense route unavailable ("
            << status.dense_error
            << "); queries run BM25-only this session.\n";
    }

    std::cout << "commands: list, del <id>  (anything else is a query)\n";

    // Session policy, not engine state: once generation fails it stays
    // off until restart, so a dead service cannot cost every query its
    // timeout. Keeping the rule in the caller is the point of the
    // degradation contract.
    bool generation_enabled = true;

    std::string query;
    while (true) {
        std::cout << "query> ";
        if (!std::getline(std::cin, query)) break;
        if (query.empty()) continue;

        if (query == "list") {
            for (const DocInfo& d : engine.documents()) {
                std::cout << d.id
                    << (d.deleted ? "  [deleted]  " : "  ")
                    << d.bytes << " bytes  "
                    << d.path << "\n";
            }
            continue;
        }

        if (query.rfind("del ", 0) == 0) {
            const std::string arg = query.substr(4);
            bool digits = !arg.empty();
            for (std::size_t k = 0; k < arg.size(); ++k)
                if (arg[k] < '0' || arg[k] > '9') digits = false;
            if (!digits) {
                std::cout << "usage: del <doc id>\n";
                continue;
            }
            int id = 0;
            for (std::size_t k = 0; k < arg.size(); ++k)
                id = id * 10 + (arg[k] - '0');

            // Cheap pre-check purely so the report lands at the right
            // moment; the engine performs the authoritative check and
            // rebuilds after it.
            std::string path;
            {
                const std::vector<DocInfo> docs = engine.documents();
                if (id < 0 || static_cast<std::size_t>(id) >= docs.size()) {
                    std::cout << "no document with id " << id << "\n";
                    continue;
                }
                if (docs[static_cast<std::size_t>(id)].deleted) {
                    std::cout << docs[static_cast<std::size_t>(id)].path
                        << " is already deleted\n";
                    continue;
                }
                path = docs[static_cast<std::size_t>(id)].path;
            }

            bool deleted_line_done = false;
            bool delete_progress_open = false;
            const MutationOutcome mutation = engine.remove_document(id,
                [&engine, &path, &deleted_line_done, &delete_progress_open](
                    const EngineProgress& p) {
                    if (p.stage == "index" && !deleted_line_done) {
                        // BM25 is rebuilt and saved; the dense side starts
                        // next. The old inline handler printed its report
                        // at exactly this point.
                        std::cout << "deleted " << path << " ("
                            << engine.status().chunks
                            << " chunks remain)\n";
                        deleted_line_done = true;
                    } else if (p.stage == "vectors") {
                        print_embed_progress(p, engine.config().dense_model,
                            "after delete");
                        delete_progress_open = (p.done != 0);
                    }
                    return true;
                });

            if (delete_progress_open)
                std::cout << "\n";
            if (mutation.status != MutationStatus::OK) {
                std::cout << "delete failed\n";   // the pre-check missed it
                continue;
            }
            const EngineStatus& s = engine.status();
            if (s.dense_ready) {
                std::cout << "dense vectors: " << s.vectors_reused
                    << " reused from snapshot, " << s.vectors_embedded
                    << " embedded (after delete)\n";
                std::cout << "dense route rebuilt: " << s.vectors_total
                    << " vectors\n";
            } else if (!s.dense_error.empty()) {
                std::cerr << "Warning: dense route unavailable ("
                    << s.dense_error << ").\n";
            }
            continue;
        }

        // Stage 3: retrieval. Fast, and printed BEFORE generation so
        // the user reads results while the model is still thinking.
        QueryOutcome outcome = engine.retrieve(query);

        if (!outcome.dense_error.empty()) {
            std::cerr << "Warning: dense route failed ("
                << outcome.dense_error << "); BM25-only for this query.\n";
        }
        if (outcome.hits.empty()) {
            std::cout << "(no results)\n";
            continue;
        }

        if (outcome.hybrid) {
            for (const QueryHit& h : outcome.hits) {
                std::cout << "[" << h.rank << "] "
                    << "rrf=" << h.rrf << "  "
                    << "bm25=" << score_or_dash(h.bm25) << "  "
                    << "vec=" << score_or_dash(h.dense) << "  "
                    << "doc=" << h.doc_path << "\n"
                    << "    " << h.text << "\n";
            }
        } else {
            for (const QueryHit& h : outcome.hits) {
                std::cout << "[" << h.rank << "] "
                    << "score=" << h.rrf << "  "
                    << "doc=" << h.doc_path << "\n"
                    << "    " << h.text << "\n";
            }
        }

        // Stage 4: the grounded answer. Slow, so it comes last.
        if (generation_enabled)
            engine.generate(query, outcome);
        if (!outcome.generation_attempted)
            continue;
        if (!outcome.generation_ok) {
            generation_enabled = false;   // session disable, as before
            std::cerr << "Warning: generation unavailable ("
                << outcome.generation_error
                << "); retrieval-only answers for the rest of this "
                "session.\n";
            continue;
        }
        std::cout << "\n--- answer (" << outcome.generation_model << ", "
            << outcome.generation_seconds << "s) ---\n"
            << outcome.answer << "\n";
        if (!outcome.citations.empty()) {
            std::cout << "--- sources ---\n";
            for (const Citation& c : outcome.citations)
                std::cout << "[" << c.n << "] " << c.doc_path
                << " (chunk " << c.chunk_id << ")\n";
        }
    }

    return 0;
}
