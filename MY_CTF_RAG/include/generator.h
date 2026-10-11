// generator.h -- the only bridge to the chat/completion service
// (contract #14).
//
// Role: a list of chat messages in, one UTF-8 answer string out. This
// is the single module that knows about HTTP, JSON, endpoints or model
// names on the generation side; context assembly (context.h) and the
// frontends see nothing but ChatMessage vectors and plain strings.
//
// Mirrors embedder.h on purpose: provider replaceability is the same
// engineering seam (Ollama /api/chat today, llama.cpp
// /v1/chat/completions in M4 step 5), and tests inject deterministic
// fakes so unit tests never need a running service.

#pragma once

#include <memory>
#include <string>
#include <vector>

// Registered in docs/PARAMS.md. CPU generation of a grounded answer can
// take tens of seconds: be generous.
constexpr const char* GENERATOR_DEFAULT_ENDPOINT = "http://localhost:11434";
constexpr int GENERATOR_DEFAULT_TIMEOUT_SECONDS = 300;

enum class ChatRole { SYSTEM, USER, ASSISTANT };

struct ChatMessage {
    ChatRole role;
    std::string text;
};

// Runtime configuration. model_id is deliberately NOT a compile-time
// constant: the generation A/B (qwen3:1.7b vs qwen3:4b-instruct) has
// not been decided yet; the winner's name becomes GENERATION_MODEL in
// PARAMS.md once decided (the same pattern EmbedderConfig used in M3).
struct GeneratorConfig {
    std::string endpoint;       // empty -> GENERATOR_DEFAULT_ENDPOINT
    std::string model_id;       // required, e.g. "qwen3:4b-instruct"
    int timeout_seconds = 0;    // <= 0  -> GENERATOR_DEFAULT_TIMEOUT_SECONDS
    double temperature = 0.2;   // faithful summary, not poetry
    int max_output_tokens = 512;
};

class LLMProvider {
public:
    virtual ~LLMProvider() = default;

    // One full, non-streaming turn: the whole answer or an exception.
    // Streaming is deliberately out of scope for M4 v1 (chunked HTTP,
    // cross-thread token signals and cancellation are M4.1 territory).
    virtual std::string
    chat(const std::vector<ChatMessage>& messages) = 0;

    // Identity, stable for the provider's lifetime.
    virtual std::string model_id() const = 0;
};

// The real implementation (Ollama POST /api/chat over localhost HTTP).
// Third-party headers (cpp-httplib, nlohmann/json) live in the .cpp
// only; this header stays free of them.
//
// Contract clauses:
// 1. Input/output UTF-8; messages are carried VERBATIM. The provider
//    applies no prompt shaping of any kind -- no prefixes, no templates,
//    no hidden system text. The RAG prompt template is context.h's
//    single source of truth (the inverse of embedder clause 1, and for
//    the same reason: exactly one owner per frozen policy).
// 2. messages order is preserved strictly. An empty messages vector is
//    a caller bug: it throws std::runtime_error and performs no
//    network call.
// 3. The returned string is the model's final answer only. Reasoning /
//    thinking traces the backend might emit are suppressed inside the
//    provider (Ollama: "think": false), so callers never have to parse
//    think blocks out of an answer.
// 4. Failures -- service unreachable, timeout, non-200 response,
//    malformed body, missing message content -- throw
//    std::runtime_error with a human-readable message. Partial results
//    are never returned. No automatic retry in v1. An answer truncated
//    by max_output_tokens is NOT a failure; it is returned as-is.
// 5. Generation parameters come from GeneratorConfig only. The provider
//    translates them into backend-specific names internally
//    (Ollama: options.temperature / options.num_predict; a future
//    llama.cpp provider: temperature / max_tokens). model_id() never
//    changes during the provider's lifetime.
// 6. make_ollama_chat_provider() performs no network I/O. The first
//    real chat call is the health check; fail-fast lands there.
// 7. The provider is stateless and generation-only: it keeps no
//    conversation history (history belongs to the caller, never to the
//    provider -- otherwise console, GUI, tests and future backends all
//    inherit lifetime bugs), it knows nothing about RAG, chunks or
//    citations, and no retrieval-layer module may include or depend on
//    this header -- only frontends, tooling and tests. Test fakes
//    subclassing LLMProvider are first-class citizens.
std::unique_ptr<LLMProvider>
make_ollama_chat_provider(const GeneratorConfig& cfg);
