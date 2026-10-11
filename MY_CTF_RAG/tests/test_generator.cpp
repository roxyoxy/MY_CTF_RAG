// test_generator.cpp -- contract tests for generator.h (M4 step 1).
// Offline only: no running Ollama is required or contacted. The real
// provider is pointed at a port nothing listens on (connection refused
// is immediate and deterministic); every green path goes through
// FakeChatProvider, a first-class citizen of the interface (clause 7).
// Live-service smoke (think:false behavior per model, latency) stays a
// manual step-0 acceptance item.
#include "check.h"
#include "generator.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class FakeChatProvider final : public LLMProvider {
public:
    // Records the message count and the last message text, so callers
    // can observe order and content through the base reference.
    std::string chat(const std::vector<ChatMessage>& messages) override {
        seen_count_ = messages.size();
        if (!messages.empty())
            seen_last_ = messages.back().text;
        return "fake answer";
    }

    std::string model_id() const override { return "fake-chat-model"; }

    std::size_t seen_count_ = 0;
    std::string seen_last_;
};

bool contains(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

GeneratorConfig dead_endpoint_cfg() {
    GeneratorConfig cfg;
    cfg.endpoint = "http://localhost:1";
    cfg.model_id = "qwen3:4b-instruct";
    return cfg;
}

}  // namespace

int main() {
    // Clause 4: unreachable service throws runtime_error naming the
    // endpoint. Port 1: nothing listens, refused without delay.
    {
        auto p = make_ollama_chat_provider(dead_endpoint_cfg());
        bool threw = false;
        std::string what;
        try {
            (void)p->chat({{ChatRole::USER, "hi"}});
        } catch (const std::runtime_error& e) {
            threw = true;
            what = e.what();
        }
        check(threw, "unreachable throws runtime_error");
        check(contains(what, "unreachable"), "message says unreachable");
        check(contains(what, "localhost:1"), "message names the endpoint");
    }

    // Clause 2: empty messages is a caller bug -- throws before any
    // request could be built.
    {
        auto p = make_ollama_chat_provider(dead_endpoint_cfg());
        bool threw = false;
        try {
            (void)p->chat({});
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "empty messages rejected");
    }

    // Clauses 5/6: construction performs no network I/O (dead endpoint,
    // no throw) and identity is stable for the lifetime.
    {
        auto p = make_ollama_chat_provider(dead_endpoint_cfg());
        check(p->model_id() == "qwen3:4b-instruct", "model_id passthrough");
    }

    // Empty model_id is rejected at construction (fail-fast before any
    // request could be built).
    {
        GeneratorConfig cfg = dead_endpoint_cfg();
        cfg.model_id.clear();
        bool threw = false;
        try {
            make_ollama_chat_provider(cfg);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "empty model_id rejected at construction");
    }

    // Non-positive max_output_tokens is rejected at construction.
    {
        GeneratorConfig cfg = dead_endpoint_cfg();
        cfg.max_output_tokens = 0;
        bool threw = false;
        try {
            make_ollama_chat_provider(cfg);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "zero max_output_tokens rejected at construction");
    }

    // Clause 7: fakes subclassing LLMProvider are first-class; the
    // whole interface is consumable through the base reference, with
    // message order and content observable at the seam.
    {
        FakeChatProvider fake;
        LLMProvider& p = fake;
        const std::string answer = p.chat({
            {ChatRole::SYSTEM, "sys"},
            {ChatRole::USER, "hello"},
            {ChatRole::ASSISTANT, "previous turn"},
            {ChatRole::USER, "world"},
        });
        check(answer == "fake answer", "fake chat answer");
        check(fake.seen_count_ == 4, "fake sees all four messages");
        check(fake.seen_last_ == "world", "message order preserved");
        check(p.model_id() == "fake-chat-model",
              "fake identity passthrough");
    }

    return test_summary();
}
