#include "generator.h"

// Both headers are warning-clean under /W4 with this toolchain
// (verified 2026-10-10); the push/pop pair is kept as a guard.
#ifdef _MSC_VER
#pragma warning(push)
#endif
#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

const char* role_name(ChatRole role) {
    switch (role) {
    case ChatRole::SYSTEM:    return "system";
    case ChatRole::USER:      return "user";
    case ChatRole::ASSISTANT: return "assistant";
    }
    return "user";
}

}  // namespace

class OllamaChatProvider final : public LLMProvider {
public:
    explicit OllamaChatProvider(const GeneratorConfig& cfg)
        : endpoint_(cfg.endpoint.empty()
              ? GENERATOR_DEFAULT_ENDPOINT
              : cfg.endpoint),
          timeout_s_(cfg.timeout_seconds <= 0
              ? GENERATOR_DEFAULT_TIMEOUT_SECONDS
              : cfg.timeout_seconds),
          model_id_(cfg.model_id),
          temperature_(cfg.temperature),
          max_output_tokens_(cfg.max_output_tokens) {
        if (model_id_.empty())
            throw std::runtime_error(
                "generation model_id must not be empty");
        if (max_output_tokens_ <= 0)
            throw std::runtime_error(
                "generation max_output_tokens must be positive");
    }

    std::string chat(const std::vector<ChatMessage>& messages) override;

    std::string model_id() const override {
        return model_id_;
    }

private:
    std::string endpoint_;
    int timeout_s_;
    std::string model_id_;
    double temperature_;
    int max_output_tokens_;
};

std::string OllamaChatProvider::chat(
    const std::vector<ChatMessage>& messages) {
    if (messages.empty())
        throw std::runtime_error(
            "chat requires at least one message");

    nlohmann::json wire_messages = nlohmann::json::array();
    for (const ChatMessage& m : messages)
        wire_messages.push_back(nlohmann::json{
            {"role", role_name(m.role)},
            {"content", m.text}});

    // "think": false -- clause 3. Grounded summary answering on a CPU
    // must not pay for reasoning traces; hybrid-thinking qwen3 models
    // emit them by default. Non-thinking models treat false as their
    // natural state (step-0 smoke pins both behaviors).
    // "stream": false -- /api/chat defaults to NDJSON streaming (one
    // JSON object per line); parsing needs a single body. The embed
    // endpoint is non-streaming by default, so embedder.cpp never met
    // this trap (caught live in the first end-to-end smoke).
    const nlohmann::json request = {
        {"model", model_id_},
        {"messages", wire_messages},
        {"think", false},
        {"stream", false},
        {"options", {
            {"temperature", temperature_},
            {"num_predict", max_output_tokens_},
            {"num_ctx", GENERATOR_NUM_CTX}
        }}
    };

    httplib::Client client(endpoint_);
    client.set_connection_timeout(timeout_s_, 0);
    client.set_read_timeout(timeout_s_, 0);
    const auto response = client.Post(
        "/api/chat", request.dump(), "application/json");

    if (!response) {
        throw std::runtime_error(
            "chat service unreachable at " + endpoint_);
    }
    if (response->status != 200) {
        throw std::runtime_error(
            "chat service returned HTTP " +
            std::to_string(response->status) + ": " +
            response->body.substr(0, 200));
    }

    nlohmann::json body;
    try {
        body = nlohmann::json::parse(response->body);
    } catch (const nlohmann::json::exception&) {
        throw std::runtime_error(
            "malformed response from chat service: invalid JSON");
    }

    if (!body.is_object() ||
        !body.contains("message") ||
        !body["message"].is_object() ||
        !body["message"].contains("content") ||
        !body["message"]["content"].is_string()) {
        throw std::runtime_error(
            "malformed response from chat service: "
            "missing message content");
    }

    return body["message"]["content"].get<std::string>();
}

std::unique_ptr<LLMProvider>
make_ollama_chat_provider(const GeneratorConfig& cfg) {
    return std::make_unique<OllamaChatProvider>(cfg);
}
