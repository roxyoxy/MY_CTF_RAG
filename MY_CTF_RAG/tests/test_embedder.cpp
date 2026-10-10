// test_embedder.cpp -- contract tests for embedder.h (M3 T8).
// Offline only: no running Ollama is required or contacted. The real
// provider is pointed at a port nothing listens on (connection refused
// is immediate and deterministic); every green path goes through
// FakeProvider, a first-class citizen of the interface (clause 7).
// Live-service smoke (dim, norms, 404) stays a manual acceptance step.
#include "check.h"
#include "embedder.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class FakeProvider final : public EmbedProvider {
public:
    std::vector<std::vector<float>>
    embed_documents(const std::vector<std::string>& texts) override {
        // Second component carries the input index: order drift is
        // observable by the caller.
        std::vector<std::vector<float>> out;
        out.reserve(texts.size());
        for (std::size_t i = 0; i < texts.size(); ++i)
            out.push_back({1.0f, static_cast<float>(i)});
        return out;
    }

    std::vector<float> embed_query(const std::string&) override {
        return {1.0f, 0.0f};
    }

    std::string model_id() const override { return "fake-model"; }
    std::string embedding_policy() const override { return "none"; }
};

bool contains(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

}  // namespace

int main() {
    // Clause 4: unreachable service throws runtime_error naming the
    // endpoint. Port 1: nothing listens, refused without delay.
    {
        EmbedderConfig cfg;
        cfg.endpoint = "http://localhost:1";
        cfg.model_id = "bge-m3";
        auto p = make_ollama_provider(cfg);
        bool threw = false;
        std::string what;
        try {
            (void)p->embed_query("x");
        } catch (const std::runtime_error& e) {
            threw = true;
            what = e.what();
        }
        check(threw, "unreachable throws runtime_error");
        check(contains(what, "unreachable"), "message says unreachable");
        check(contains(what, "localhost:1"), "message names the endpoint");
    }

    // Clause 2: empty batch -> empty output, no network call at all.
    {
        EmbedderConfig cfg;
        cfg.endpoint = "http://localhost:1";
        cfg.model_id = "bge-m3";
        auto p = make_ollama_provider(cfg);
        check(p->embed_documents({}).empty(), "empty batch returns empty");
    }

    // Clauses 5/6: construction does no network I/O (dead endpoint,
    // no throw so far) and identity is keyed on the model family:
    // the qwen3 instruction policy pair vs the plain default.
    {
        EmbedderConfig q;
        q.endpoint = "http://localhost:1";
        q.model_id = "qwen3-embedding:0.6b";
        auto pq = make_ollama_provider(q);
        check(pq->model_id() == "qwen3-embedding:0.6b",
              "model_id passthrough");
        check(pq->embedding_policy() == "qwen3-instruct-v1",
              "qwen3 family policy id");
        EmbedderConfig b;
        b.endpoint = "http://localhost:1";
        b.model_id = "bge-m3";
        auto pb = make_ollama_provider(b);
        check(pb->embedding_policy() == "none", "non-qwen3 policy none");
    }

    // Empty model_id is rejected at construction (fail-fast before
    // any request could be built).
    {
        EmbedderConfig cfg;
        cfg.endpoint = "http://localhost:1";
        bool threw = false;
        try {
            make_ollama_provider(cfg);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "empty model_id rejected at construction");
    }

    // Clause 7: fakes subclassing EmbedProvider are first-class; the
    // whole interface is consumable through the base pointer only.
    {
        std::unique_ptr<EmbedProvider> p = std::make_unique<FakeProvider>();
        check(p->embed_query("q").size() == 2, "fake polymorphic query");
        const std::vector<std::vector<float>> docs =
            p->embed_documents({"a", "b", "c"});
        check(docs.size() == 3, "fake polymorphic documents size");
        check(docs.size() == 3 && docs[0][1] == 0.0f &&
                  docs[1][1] == 1.0f && docs[2][1] == 2.0f,
              "output order follows input order (fake)");
        check(p->model_id() == "fake-model" &&
                  p->embedding_policy() == "none",
              "fake identity passthrough");
    }

    return test_summary();
}
