#include "embedder.h"

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

#include <cstddef>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr char QWEN3_MODEL_PREFIX[] = "qwen3";
constexpr char QWEN3_QUERY_PREFIX[] =
    "Instruct: Given a CTF search query, retrieve relevant "
    "writeup passages that answer the query\nQuery: ";
constexpr char QWEN3_EMBEDDING_POLICY[] = "qwen3-instruct-v1";
constexpr char DEFAULT_EMBEDDING_POLICY[] = "none";

bool uses_qwen3_instruction(const std::string& model_id) {
    return model_id.compare(
        0, sizeof(QWEN3_MODEL_PREFIX) - 1, QWEN3_MODEL_PREFIX) == 0;
}

const char* embedding_policy_for(const std::string& model_id) {
    return uses_qwen3_instruction(model_id)
        ? QWEN3_EMBEDDING_POLICY
        : DEFAULT_EMBEDDING_POLICY;
}

}

class OllamaProvider final : public EmbedProvider {
public:
    explicit OllamaProvider(const EmbedderConfig& cfg)
        : endpoint_(cfg.endpoint.empty()
              ? EMBEDDER_DEFAULT_ENDPOINT
              : cfg.endpoint),
          timeout_s_(cfg.timeout_seconds <= 0
              ? EMBEDDER_DEFAULT_TIMEOUT_SECONDS
              : cfg.timeout_seconds),
          model_id_(cfg.model_id),
          policy_(embedding_policy_for(cfg.model_id)) {
        if (model_id_.empty())
            throw std::runtime_error("embedding model_id must not be empty");
    }

    std::vector<std::vector<float>>
    embed_documents(const std::vector<std::string>& texts) override;

    std::vector<float>
    embed_query(const std::string& text) override;

    std::string model_id() const override {
        return model_id_;
    }

    std::string embedding_policy() const override {
        return policy_;
    }

private:
    std::vector<std::vector<float>>
    request_embeddings(const std::vector<std::string>& texts) const;

    std::string endpoint_;
    int timeout_s_;
    std::string model_id_;
    std::string policy_;
};

std::vector<std::vector<float>>
OllamaProvider::request_embeddings(
    const std::vector<std::string>& texts) const {
    if (texts.empty())
        return {};

    const nlohmann::json request = {
        {"model", model_id_},
        {"input", texts}
    };

    httplib::Client client(endpoint_);
    client.set_connection_timeout(timeout_s_, 0);
    client.set_read_timeout(timeout_s_, 0);
    const auto response = client.Post(
        "/api/embed", request.dump(), "application/json");

    if (!response) {
        throw std::runtime_error(
            "embedding service unreachable at " + endpoint_);
    }
    if (response->status != 200) {
        throw std::runtime_error(
            "embedding service returned HTTP " +
            std::to_string(response->status) + ": " +
            response->body.substr(0, 200));
    }

    nlohmann::json body;
    try {
        body = nlohmann::json::parse(response->body);
    } catch (const nlohmann::json::exception&) {
        throw std::runtime_error(
            "malformed response from embedding service: invalid JSON");
    }

    if (!body.is_object() ||
        !body.contains("embeddings") ||
        !body["embeddings"].is_array() ||
        body["embeddings"].size() != texts.size()) {
        throw std::runtime_error(
            "malformed response from embedding service: invalid embeddings");
    }

    std::vector<std::vector<float>> output;
    output.reserve(texts.size());
    std::size_t expected_dimension = 0;

    try {
        for (const auto& embedding : body["embeddings"]) {
            if (!embedding.is_array() || embedding.empty()) {
                throw std::runtime_error(
                    "malformed response from embedding service: empty vector");
            }

            if (expected_dimension == 0)
                expected_dimension = embedding.size();
            else if (embedding.size() != expected_dimension) {
                throw std::runtime_error(
                    "malformed response from embedding service: "
                    "inconsistent vector dimensions");
            }

            std::vector<float> vector;
            vector.reserve(embedding.size());
            double squared_norm = 0.0;
            for (const auto& value : embedding) {
                if (!value.is_number()) {
                    throw std::runtime_error(
                        "malformed response from embedding service: "
                        "non-numeric vector value");
                }

                const float component = value.get<float>();
                if (!std::isfinite(component)) {
                    throw std::runtime_error(
                        "malformed response from embedding service: "
                        "non-finite vector value");
                }
                vector.push_back(component);
                squared_norm += static_cast<double>(component) * component;
            }

            const double norm = std::sqrt(squared_norm);
            if (!std::isfinite(norm) || norm == 0.0) {
                throw std::runtime_error(
                    "malformed response from embedding service: "
                    "zero or invalid vector norm");
            }

            for (float& component : vector) {
                component = static_cast<float>(
                    static_cast<double>(component) / norm);
            }
            output.push_back(std::move(vector));
        }
    } catch (const nlohmann::json::exception&) {
        throw std::runtime_error(
            "malformed response from embedding service: invalid vector data");
    }

    return output;
}

std::vector<std::vector<float>>
OllamaProvider::embed_documents(
    const std::vector<std::string>& texts) {
    return request_embeddings(texts);
}

std::vector<float>
OllamaProvider::embed_query(const std::string& text) {
    std::string query = text;
    if (uses_qwen3_instruction(model_id_))
        query.insert(0, QWEN3_QUERY_PREFIX);

    std::vector<std::vector<float>> embeddings =
        request_embeddings(std::vector<std::string>{std::move(query)});
    return std::move(embeddings.front());
}

std::unique_ptr<EmbedProvider>
make_ollama_provider(const EmbedderConfig& cfg) {
    return std::make_unique<OllamaProvider>(cfg);
}
