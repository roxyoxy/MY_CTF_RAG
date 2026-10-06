// embedder.h -- the only bridge to the embedding service (contract #9).
//
// Role: raw UTF-8 text in, unit-length float vectors out. This is the
// single module that knows about HTTP, JSON, endpoints or model names;
// every other M3 module (vector_index / rrf / vector_persist / hnsw)
// sees nothing but std::vector<float>.
//
// First virtual interface in this codebase, on purpose: provider
// replaceability is the core engineering seam of the M3 route (Ollama
// today, llama.cpp server in M4), and tests inject deterministic fakes
// so unit tests never need a running service.

#pragma once

#include <memory>
#include <string>
#include <vector>

// Registered in docs/PARAMS.md. Batch inference of a whole corpus on a
// local CPU can take tens of seconds: be generous.
constexpr const char* EMBEDDER_DEFAULT_ENDPOINT = "http://localhost:11434";
constexpr int EMBEDDER_DEFAULT_TIMEOUT_SECONDS = 180;

// Runtime configuration. model_id is deliberately NOT a compile-time
// constant: the A/B experiment (bge-m3 vs qwen3-embedding:0.6b) has not
// been decided yet, so both are legal configurations; the winner's name
// becomes the documented default in PARAMS.md once decided.
struct EmbedderConfig {
    std::string endpoint;      // empty -> EMBEDDER_DEFAULT_ENDPOINT
    std::string model_id;      // required, e.g. "bge-m3"
    int timeout_seconds = 0;   // <= 0  -> EMBEDDER_DEFAULT_TIMEOUT_SECONDS
};

class EmbedProvider {
public:
    virtual ~EmbedProvider() = default;

    // Corpus side, batched: one call may carry every chunk of the
    // library (the Ollama implementation sends them in one request).
    virtual std::vector<std::vector<float>>
    embed_documents(const std::vector<std::string>& texts) = 0;

    // Query side, single text. Kept separate from embed_documents on
    // purpose: some models (e.g. qwen3-embedding) take an instruction
    // prefix on the query side only; the document side does not.
    virtual std::vector<float>
    embed_query(const std::string& text) = 0;

    // Identity, stable for the provider's lifetime. Authoritative
    // source for the dense snapshot identity (vector_persist.h).
    virtual std::string model_id() const = 0;
    virtual std::string embedding_policy() const = 0;
};

// The real implementation (Ollama POST /api/embed over localhost HTTP).
// Third-party headers (cpp-httplib, nlohmann/json) live in the .cpp
// only; this header stays free of them.
//
// Contract clauses:
// 1. Callers pass raw UTF-8 text; any model-specific prefixing is
//    applied internally, as a frozen document-side / query-side pair.
//    Callers never add prefixes themselves.
// 2. Output order follows input order: output[i] embeds texts[i]. An
//    empty input vector returns an empty output vector and performs no
//    network call at all.
// 3. Every returned vector is unit length (normalized internally when
//    the backend did not). All vectors of one call share one dimension;
//    the dimension is stable across calls for a given model. Consumers
//    may therefore use the plain dot product as cosine similarity.
// 4. Failures -- service unreachable, timeout, non-200 response,
//    malformed body, unknown model -- throw std::runtime_error with a
//    human-readable message. Partial results are never returned. No
//    automatic retry in v1.
// 5. Identity: model_id() and embedding_policy() never change during
//    the provider's lifetime. The dimension is NOT asked from the
//    provider: it is read from output vectors. Identity asks the
//    provider, size asks the output.
// 6. make_ollama_provider() performs no network I/O. The first real
//    embed call is the health check; fail-fast lands there.
// 7. Layering red line: no retrieval-layer module may include or depend
//    on this header -- only main, tooling and tests. Test fakes
//    subclassing EmbedProvider are first-class citizens of this
//    interface.
std::unique_ptr<EmbedProvider>
make_ollama_provider(const EmbedderConfig& cfg);
