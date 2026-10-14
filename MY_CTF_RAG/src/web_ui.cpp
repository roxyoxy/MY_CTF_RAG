// web_ui.cpp
// Third front end: the local web UI.
//
// The engine does all the work (engine.h, contract #16); this file is a
// thin HTTP shell. It owns no pipeline state and no query policy --
// only route handlers that translate between JSON and RagEngine. The
// console (src/main.cpp) and the Qt window (gui/) are the same kind of
// shell over the same engine, which is the whole point of the
// extraction.
//
// Endpoints (all JSON):
//   GET  /api/status   engine facts (docs / chunks / mode / models)
//   POST /api/search   {query, top_k?} -> ranked results
//   POST /api/ask      {query}         -> results + answer + sources
//   GET  /*            static files from web/
//
// Run it from the code area (the working directory must contain data/,
// index.bin, vector.bin and web/).

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>

#ifdef _WIN32
#define NOMINMAX             // keep windows.h min/max macros out of std::min
#define WIN32_LEAN_AND_MEAN  // windows.h must NOT pull in winsock.h:
#include <windows.h>         // httplib.h needs winsock2.h, and the two
#endif                        // cannot coexist in one translation unit

#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"

#include "engine.h"

using json = nlohmann::json;

namespace {

constexpr const char* kBindAddress = "127.0.0.1";  // a local tool: never wider
constexpr int kPort = 18080;
constexpr const char* kWebRoot = "web";

// One ranked hit. bm25 / vec are null when that route did not recall the
// chunk -- the page renders a muted dot there, which is what makes
// "which route found this" visible at a glance.
json hit_json(const QueryHit& hit) {
    json out{
        {"rank", hit.rank},
        {"chunk_id", hit.chunk_id},
        {"rrf", hit.rrf},
        {"bm25", nullptr},
        {"vec", nullptr},
        {"doc", hit.doc_path},
        {"text", hit.text},
    };
    if (hit.bm25.present)
        out["bm25"] = hit.bm25.score;
    if (hit.dense.present)
        out["vec"] = hit.dense.score;
    return out;
}

// The retrieval half of every answer: mode, timing, ranked hits.
json base_json(const QueryOutcome& outcome) {
    json results = json::array();
    for (const QueryHit& hit : outcome.hits)
        results.push_back(hit_json(hit));
    return json{
        {"mode", outcome.hybrid ? "hybrid" : "bm25-only"},
        {"ms", outcome.retrieval_seconds * 1000.0},
        {"results", results},
    };
}

// Reads {query, top_k?} out of the request body. Fills `error` and
// returns false when the request is unusable.
bool read_query(const httplib::Request& req, std::string& query,
                std::size_t& top_k, json& error) {
    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        error = json{{"error", "bad json"}};
        return false;
    }
    if (!body.is_object()) {
        error = json{{"error", "bad json"}};
        return false;
    }
    query = body.value("query", std::string());
    if (query.empty()) {
        error = json{{"error", "empty query"}};
        return false;
    }
    const int requested = body.value("top_k", 0);
    top_k = requested > 0 ? static_cast<std::size_t>(requested)
                          : static_cast<std::size_t>(TOP_K);
    return true;
}

void send_json(httplib::Response& res, const json& body, int status = 200) {
    res.status = status;
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    RagEngine engine;
    EngineStatus status;
    try {
        status = engine.open(EngineConfig{});
    } catch (const std::exception& e) {
        std::cerr << "startup failed: " << e.what() << "\n";
        return 1;
    }
    std::cout << "corpus: " << status.documents << " docs / "
        << status.chunks << " chunks\n";
    if (status.dense_ready) {
        std::cout << "dense route ready: " << status.vectors_total
            << " vectors, dim " << status.dimension << "\n";
    } else if (!status.dense_error.empty()) {
        std::cout << "dense route unavailable (" << status.dense_error
            << "), BM25-only this session\n";
    }

    httplib::Server server;

    // The engine is NOT thread-safe by contract -- "one thread at a
    // time". So the server gets exactly one worker: requests serialize
    // themselves and no lock is needed anywhere. For a single-user local
    // tool that is the right trade, and it makes the contract literal
    // instead of aspirational.
    server.new_task_queue = [] { return new httplib::ThreadPool(1); };

    server.Get("/api/status", [&engine](const httplib::Request&,
                                        httplib::Response& res) {
        const EngineStatus& s = engine.status();
        send_json(res, json{
            {"docs", s.documents},
            {"chunks", s.chunks},
            {"dense_ready", s.dense_ready},
            {"mode", s.dense_ready ? "hybrid" : "bm25-only"},
            {"dense_model", s.dense_model_id},
            {"gen_model", engine.config().generation_model},
            {"generation_ready", s.generation_ready},
        });
    });

    server.Post("/api/search", [&engine](const httplib::Request& req,
                                         httplib::Response& res) {
        std::string query;
        std::size_t top_k = 0;
        json error;
        if (!read_query(req, query, top_k, error)) {
            send_json(res, error, 400);
            return;
        }
        send_json(res, base_json(engine.retrieve(query, top_k)));
    });

    server.Post("/api/ask", [&engine](const httplib::Request& req,
                                      httplib::Response& res) {
        std::string query;
        std::size_t top_k = 0;
        json error;
        if (!read_query(req, query, top_k, error)) {
            send_json(res, error, 400);
            return;
        }

        QueryOutcome outcome = engine.retrieve(query, top_k);
        json out = base_json(outcome);

        // Retrieval and generation are two engine calls on purpose: the
        // page gets its hits even when the model is slow, unavailable or
        // fails. generate() itself skips when there is nothing to ground,
        // so answer stays null and the retrieval list IS the answer --
        // the degradation rule, visible in the payload instead of hidden
        // behind an error status.
        out["answer"] = nullptr;
        out["sources"] = json::array();
        engine.generate(query, outcome);
        if (outcome.generation_attempted) {
            out["gen_model"] = outcome.generation_model;
            out["gen_seconds"] = outcome.generation_seconds;
            if (outcome.generation_ok) {
                out["answer"] = outcome.answer;
                json sources = json::array();
                for (const Citation& c : outcome.citations) {
                    sources.push_back(json{
                        {"n", c.n},
                        {"source", c.doc_path},
                        {"chunk_id", c.chunk_id},
                    });
                }
                out["sources"] = sources;
            } else {
                out["gen_error"] = outcome.generation_error;
            }
        }
        send_json(res, out);
    });

    server.set_mount_point("/", kWebRoot);

    std::cout << "TraceRAG web UI: http://" << kBindAddress << ":" << kPort
        << "\n";
    if (!server.listen(kBindAddress, kPort)) {
        std::cerr << "cannot bind " << kBindAddress << ":" << kPort
            << " -- is another instance still running?\n";
        return 1;
    }
    return 0;
}
