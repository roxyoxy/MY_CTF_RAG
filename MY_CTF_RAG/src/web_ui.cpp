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
// index.bin, vector.bin and web/). It runs in the foreground, prints the
// URL it is serving, and stops on Ctrl+C -- or on Ctrl+Break, or on the
// window being closed, all three of which land in the same handler and
// close the listening socket before returning.

#include <atomic>
#include <csignal>
#include <cstddef>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

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
        // Where the chunk sits in its document. The page needs the
        // byte range to open the full text and highlight the match.
        {"doc_id", hit.location.document_id},
        {"begin", hit.location.begin},
        {"end", hit.location.end},
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

// Startup runs on its own thread so the server can answer immediately.
// A first run with no vector.bin embeds the whole corpus -- minutes on
// CPU -- and the browser must be able to load the page and watch
// progress instead of finding nothing listening.
//
// Nothing here touches the engine: the progress callback writes these
// atomics from the startup thread, and handlers read them. The engine
// itself is only ever reached by ONE thread at a time -- the startup
// thread before ready, the search/ask handlers after.
struct StartupState {
    std::atomic<bool> ready{false};
    std::atomic<bool> failed{false};
    std::atomic<int> stage{0};        // 0 plan, 1 index, 2 vectors, 3 done
    std::atomic<unsigned long long> done{0};
    std::atomic<unsigned long long> total{0};
    std::string error;                // written before failed is published
};

int stage_code(const std::string& stage) {
    if (stage == "index")
        return 1;
    if (stage == "vectors")
        return 2;
    return 0;
}

const char* stage_name(int stage) {
    switch (stage) {
    case 1:  return "index";
    case 2:  return "vectors";
    case 3:  return "done";
    default: return "plan";
    }
}

// What a caller gets while the engine is still coming up. `starting`
// is the flag the page keys on; the stage and counts drive its progress
// line. 503 for the data routes, 200 for /api/status -- asking how it
// is going is always a valid question.
json starting_json(const StartupState& state) {
    return json{
        {"starting", true},
        {"stage", stage_name(state.stage.load(std::memory_order_relaxed))},
        {"done", state.done.load(std::memory_order_relaxed)},
        {"total", state.total.load(std::memory_order_relaxed)},
    };
}

const json kStillStarting = json{{"error", "starting"}};

// Stopping cleanly means reaching the Server object to close the listening
// socket, which is what makes listen_after_bind() return. Neither a signal
// handler nor a console handler can capture anything, so the object is
// parked here. Both kinds of handler run on a thread the system creates
// for them, never on a serving thread, so calling into Server from one
// does not race the request loop.
httplib::Server* g_server = nullptr;

#ifdef _WIN32
// One handler covers every way a user ends a console program: Ctrl+C,
// Ctrl+Break, and closing the window. std::signal would only reach
// Ctrl+C, leaving the other two to kill the process outright -- which is
// the state this replaced, where the window could be closed while the
// server kept running.
BOOL WINAPI on_console_event(DWORD type) {
    switch (type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (g_server) {
            g_server->stop();
        }
        return TRUE;
    default:
        return FALSE;
    }
}
#else
void on_interrupt(int) {
    if (g_server) {
        g_server->stop();
    }
}
#endif

void install_shutdown_handler() {
#ifdef _WIN32
    SetConsoleCtrlHandler(on_console_event, TRUE);
#else
    std::signal(SIGINT, on_interrupt);
#endif
}

}  // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Bind before anything else. A port still held by a previous run is
    // the failure a user is most likely to hit, and it is cheap to
    // detect -- so it is detected here, where saying so costs nothing,
    // rather than after the engine has spent time loading snapshots.
    httplib::Server server;

#ifdef _WIN32
    // httplib's default socket options set SO_REUSEADDR and then
    // SO_EXCLUSIVEADDRUSE on Windows. Measured 2026-10-15: with both set,
    // a second instance bound the same port successfully and printed its
    // banner, so the check below never fired -- two servers answered on
    // one port and neither could tell. Setting the exclusive option on
    // its own is what actually refuses the second bind.
    // A generic lambda rather than naming the socket type: httplib keeps
    // socket_t out of its public namespace, and the parameter is deduced
    // from the std::function this is stored into anyway.
    server.set_socket_options([](auto sock) {
        int exclusive = 1;
        setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
            reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));
    });
#endif

    if (!server.bind_to_port(kBindAddress, kPort)) {
        std::cerr << "cannot bind " << kBindAddress << ":" << kPort
            << " -- is another instance still running?\n";
        return 1;
    }

    // Ctrl+C (and Ctrl+Break, and closing the window) stops the server
    // instead of killing the process, so the listening socket closes and
    // in-flight requests finish. The banner cannot be printed before the
    // bind, or a failed start would be preceded by "open this URL".
    g_server = &server;
    install_shutdown_handler();

    std::cout << "\n  TraceRAG web UI  ->  http://"
        << kBindAddress << ":" << kPort << "\n"
        << "  Press Ctrl+C in this window to stop the server.\n\n";
    std::cout.flush();

    RagEngine engine;
    StartupState startup;
    std::vector<DocInfo> docs_snapshot;
    std::unordered_map<int, std::string> doc_text;
    json status_snapshot = json::object();
    std::mutex engine_mutex;

    // The engine comes up on its own thread; the server starts listening
    // right away and tells callers how far along it is. The two never
    // overlap: handlers stay off the engine until ready is published, and
    // the snapshot below is written before that release.
    std::thread startup_thread([&]() {
        EngineStatus status;
        try {
            status = engine.open(EngineConfig{},
                [&startup](const EngineProgress& p) {
                    startup.stage.store(stage_code(p.stage),
                                        std::memory_order_relaxed);
                    startup.done.store(p.done, std::memory_order_relaxed);
                    startup.total.store(p.total, std::memory_order_relaxed);
                    if (p.total > 0) {
                        std::cout << "  " << p.stage << ": " << p.done
                            << "/" << p.total << "\n";
                    }
                    return true;
                });
        } catch (const std::exception& e) {
            startup.error = e.what();
            startup.failed.store(true, std::memory_order_release);
            std::cerr << "startup failed: " << e.what() << "\n";
            return;
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

        // The web UI is read-only, so everything it says about the corpus
        // is fixed for the life of the process. Snapshot it once instead
        // of asking the engine per request: a generation holds the engine
        // for tens of seconds, and a document lookup must not queue.
        docs_snapshot = engine.documents();
        for (const DocInfo& doc : docs_snapshot) {
            std::string content;
            if (engine.document_text(doc.id, content))
                doc_text.emplace(doc.id, std::move(content));
        }
        status_snapshot = json{
            {"docs", status.documents},
            {"chunks", status.chunks},
            {"dense_ready", status.dense_ready},
            {"mode", status.dense_ready ? "hybrid" : "bm25-only"},
            {"dense_model", status.dense_model_id},
            {"gen_model", engine.config().generation_model},
            {"generation_ready", status.generation_ready},
        };
        startup.stage.store(3, std::memory_order_relaxed);
        startup.ready.store(true, std::memory_order_release);
        std::cout << "engine ready -- " << docs_snapshot.size()
            << " documents\n";
    });
    startup_thread.detach();

    // Only /api/search and /api/ask touch the engine, and its contract
    // allows one thread at a time -- so those two take a mutex while the
    // snapshot-backed routes run free. The caller does the serializing,
    // exactly as the contract asks.
    server.new_task_queue = [] { return new httplib::ThreadPool(4); };

    server.Get("/api/status", [&](const httplib::Request&,
                                   httplib::Response& res) {
        if (startup.failed.load(std::memory_order_acquire)) {
            send_json(res, json{{"error", startup.error}}, 500);
            return;
        }
        if (!startup.ready.load(std::memory_order_acquire)) {
            send_json(res, starting_json(startup));   // 200: asking is always valid
            return;
        }
        send_json(res, status_snapshot);
    });

    server.Post("/api/search", [&](const httplib::Request& req,
                                         httplib::Response& res) {
        if (!startup.ready.load(std::memory_order_acquire)) {
            send_json(res, kStillStarting, 503);
            return;
        }
        std::string query;
        std::size_t top_k = 0;
        json error;
        if (!read_query(req, query, top_k, error)) {
            send_json(res, error, 400);
            return;
        }
        std::lock_guard<std::mutex> lock(engine_mutex);
        send_json(res, base_json(engine.retrieve(query, top_k)));
    });

    server.Post("/api/ask", [&](const httplib::Request& req,
                                      httplib::Response& res) {
        if (!startup.ready.load(std::memory_order_acquire)) {
            send_json(res, kStillStarting, 503);
            return;
        }
        std::string query;
        std::size_t top_k = 0;
        json error;
        if (!read_query(req, query, top_k, error)) {
            send_json(res, error, 400);
            return;
        }

        std::lock_guard<std::mutex> lock(engine_mutex);
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
                        {"doc_id", c.location.document_id},
                        {"begin", c.location.begin},
                        {"end", c.location.end},
                    });
                }
                out["sources"] = sources;
            } else {
                out["gen_error"] = outcome.generation_error;
            }
        }
        send_json(res, out);
    });

    // Full text of one document. The collapsible result rows expand
    // into this, and the detail pane renders it with the matched chunk
    // highlighted -- which is why hits carry begin/end rather than
    // only a snippet.
    server.Get("/api/doc", [&](const httplib::Request& req,
                                   httplib::Response& res) {
        if (!startup.ready.load(std::memory_order_acquire)) {
            send_json(res, kStillStarting, 503);
            return;
        }
        const std::string raw = req.get_param_value("id");
        if (raw.empty() || raw.size() > 6) {
            send_json(res, json{{"error", "bad id"}}, 400);
            return;
        }
        int id = 0;
        for (const char ch : raw) {
            if (ch < '0' || ch > '9') {
                send_json(res, json{{"error", "bad id"}}, 400);
                return;
            }
            id = id * 10 + (ch - '0');
        }
        const auto found = doc_text.find(id);
        if (found == doc_text.end()) {
            send_json(res, json{{"error", "no such document"}}, 404);
            return;
        }
        const std::string& content = found->second;
        std::string path;
        for (const DocInfo& doc : docs_snapshot) {
            if (doc.id == id) {
                path = doc.path;
                break;
            }
        }
        send_json(res, json{
            {"id", id},
            {"path", path},
            {"bytes", content.size()},
            {"content", content},
        });
    });

    server.set_mount_point("/", kWebRoot);

    // Blocks until Ctrl+C. The socket is already bound, so the only way
    // out of here is a signal, and stop() is what makes this return.
    server.listen_after_bind();
    std::cout << "server stopped.\n";
    return 0;
}
