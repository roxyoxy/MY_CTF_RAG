// test_llama_cpp.cpp -- offline tests for the llama.cpp providers (M4
// step 5). A local httplib::Server serves canned OpenAI-shaped responses
// so the wire shape (/v1/embeddings, /v1/chat/completions) is exercised
// without a real llama-server. Live smoke against a real llama-server +
// GGUF stays a manual acceptance step (like Ollama's live smoke).

#include "check.h"
#include "embedder.h"
#include "generator.h"

#ifdef _MSC_VER
#pragma warning(push)
#endif
#include "../third_party/httplib.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

bool contains(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

}  // namespace

int main() {
    // Embeddings: the provider POSTs to /v1/embeddings and parses the
    // OpenAI data[].embedding shape; 3-4-5 normalizes to 0.6 / 0.8.
    {
        httplib::Server svr;
        std::string got_path;
        std::string got_body;
        svr.Post("/v1/embeddings",
                 [&](const httplib::Request& req, httplib::Response& res) {
                     got_path = req.path;
                     got_body = req.body;
                     res.set_content(
                         "{\"data\":[{\"embedding\":[3.0,4.0],\"index\":0}]}",
                         "application/json");
                 });
        const int port = svr.bind_to_any_port("localhost");
        std::thread t([&svr]() { svr.listen_after_bind(); });
        svr.wait_until_ready();

        EmbedderConfig cfg;
        cfg.endpoint = "http://localhost:" + std::to_string(port);
        cfg.model_id = "bge-m3";
        auto p = make_llama_cpp_provider(cfg);
        std::vector<float> v;
        bool threw = false;
        try {
            v = p->embed_query("hello");
        } catch (const std::exception&) {
            threw = true;
        }

        svr.stop();
        t.join();

        check(!threw, "embed_query did not throw");
        check(got_path == "/v1/embeddings", "embeddings path /v1/embeddings");
        check(contains(got_body, "hello"), "embeddings body carries the text");
        check(v.size() == 2 && v[0] > 0.599f && v[0] < 0.601f &&
                  v[1] > 0.799f && v[1] < 0.801f,
              "embedding parsed + normalized to 0.6 / 0.8");
    }

    // Chat: the provider POSTs to /v1/chat/completions with OpenAI
    // fields (temperature / max_tokens / stream, no "think") and parses
    // choices[0].message.content.
    {
        httplib::Server svr;
        std::string got_path;
        std::string got_body;
        svr.Post("/v1/chat/completions",
                 [&](const httplib::Request& req, httplib::Response& res) {
                     got_path = req.path;
                     got_body = req.body;
                     res.set_content(
                         "{\"choices\":[{\"message\":{\"role\":\"assistant\","
                         "\"content\":\"the answer\"},\"finish_reason\":"
                         "\"stop\"}]}",
                         "application/json");
                 });
        const int port = svr.bind_to_any_port("localhost");
        std::thread t([&svr]() { svr.listen_after_bind(); });
        svr.wait_until_ready();

        GeneratorConfig cfg;
        cfg.endpoint = "http://localhost:" + std::to_string(port);
        cfg.model_id = "qwen3:4b-instruct";
        cfg.temperature = 0.3;
        cfg.max_output_tokens = 256;
        auto p = make_llama_cpp_chat_provider(cfg);
        std::string answer;
        bool threw = false;
        try {
            answer = p->chat({{ChatRole::USER, "hi"}});
        } catch (const std::exception&) {
            threw = true;
        }

        svr.stop();
        t.join();

        check(!threw, "chat did not throw");
        check(got_path == "/v1/chat/completions",
              "chat path /v1/chat/completions");
        check(contains(got_body, "\"temperature\":0.3"),
              "chat body carries temperature");
        check(contains(got_body, "\"max_tokens\":256"),
              "chat body carries max_tokens");
        check(contains(got_body, "\"stream\":false"),
              "chat body carries stream false");
        check(!contains(got_body, "think"), "chat body has no think field");
        check(answer == "the answer",
              "answer parsed from choices[0].message.content");
    }

    // Embedder: empty model_id rejected at construction.
    {
        EmbedderConfig cfg;
        cfg.endpoint = "http://localhost:1";
        bool threw = false;
        try {
            make_llama_cpp_provider(cfg);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "embedder empty model_id rejected at construction");
    }

    // Chat: zero max_output_tokens rejected at construction.
    {
        GeneratorConfig cfg;
        cfg.endpoint = "http://localhost:1";
        cfg.model_id = "m";
        cfg.max_output_tokens = 0;
        bool threw = false;
        try {
            make_llama_cpp_chat_provider(cfg);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "chat zero max_output_tokens rejected at construction");
    }

    // Embedder: unreachable endpoint throws runtime_error.
    {
        EmbedderConfig cfg;
        cfg.endpoint = "http://localhost:1";
        cfg.model_id = "m";
        auto p = make_llama_cpp_provider(cfg);
        bool threw = false;
        try {
            (void)p->embed_query("x");
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "embedder unreachable throws");
    }

    // Chat: unreachable endpoint throws runtime_error.
    {
        GeneratorConfig cfg;
        cfg.endpoint = "http://localhost:1";
        cfg.model_id = "m";
        auto p = make_llama_cpp_chat_provider(cfg);
        bool threw = false;
        try {
            (void)p->chat({{ChatRole::USER, "hi"}});
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "chat unreachable throws");
    }

    return test_summary();
}
