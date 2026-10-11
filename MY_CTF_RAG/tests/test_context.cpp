// test_context.cpp -- contract tests for context.h (M4 step 2).
// build_rag_prompt is a pure function: every case is offline and
// deterministic. Budget cases use wide margins (hundreds of bytes)
// instead of hand-counted exact block sizes -- the semantics under
// test are whole-material trimming and byte honesty, not arithmetic.
#include "check.h"
#include "context.h"

#include <string>
#include <vector>

namespace {

RagMaterial mat(int chunk_id, const std::string& source,
                const std::string& text) {
    RagMaterial m;
    m.chunk_id = chunk_id;
    m.source = source;
    m.text = text;
    return m;
}

bool contains(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

}  // namespace

int main() {
    // Clauses 1/4: rank order becomes [n] numbering, and the ledger
    // matches it exactly (used_materials[0] IS [1]).
    {
        const RagPrompt p = build_rag_prompt(
            "what is ret2libc",
            {mat(7, "pwn/stack.md", "ret2libc text A"),
             mat(3, "pwn/notepad.md", "text B"),
             mat(9, "misc/x.md", "text C")},
            CONTEXT_BYTE_BUDGET);
        check(p.used_materials.size() == 3,
              "all three fit the default budget");
        check(p.used_materials[0].chunk_id == 7 &&
                  p.used_materials[1].chunk_id == 3 &&
                  p.used_materials[2].chunk_id == 9,
              "ledger keeps rank order");
        check(contains(p.user_message, "[1]\nSource: pwn/stack.md"),
              "[1] block header");
        check(contains(p.user_message, "[2]\nSource: pwn/notepad.md"),
              "[2] block header");
        check(contains(p.user_message,
                       "<<<BEGIN MATERIAL>>>\nret2libc text A"),
              "text fenced open");
        check(contains(p.user_message, "ret2libc text A\n<<<END MATERIAL>>>"),
              "text fenced close");
        check(p.user_message.rfind("Question: what is ret2libc", 0) == 0,
              "question rendered first");
        check(p.user_message.find("text A") < p.user_message.find("text B") &&
                  p.user_message.find("text B") < p.user_message.find("text C"),
              "materials in rank order inside the prompt");
    }

    // Clause 2 boundary: budget 0 still keeps the top-ranked material
    // (degenerate guard -- non-empty input never yields empty context).
    {
        const RagPrompt p = build_rag_prompt(
            "q", {mat(1, "a.md", "AA"), mat(2, "b.md", "BB")}, 0);
        check(p.used_materials.size() == 1,
              "budget 0 keeps top material only");
        check(p.used_materials[0].chunk_id == 1,
              "top material is [1] under budget 0");
    }

    // Clause 2: the walk STOPS at the first overflow. A smaller later
    // material must not be skipped to even though it would fit.
    {
        const RagPrompt p = build_rag_prompt(
            "q",
            {mat(1, "a.md", "small"),
             mat(2, "big.md", std::string(500, 'x')),
             mat(3, "c.md", "tiny")},
            100);
        check(p.used_materials.size() == 1, "overflow stops the walk");
        check(!contains(p.user_message, "tiny"),
              "no skipping to a smaller later material");
        check(!contains(p.user_message, "big.md"),
              "overflowing material fully absent");
    }

    // Clause 2 byte honesty: same character count, different byte
    // count. 500 ASCII chars fit where 500 Han chars (1500 UTF-8
    // bytes) overflow -- a character-counting budget would pass both.
    // \uXXXX escapes keep this file pure ASCII; with /utf-8 the
    // literals carry the same UTF-8 bytes as the real corpus text.
    {
        std::string cjk;
        for (int i = 0; i < 500; ++i)
            cjk += "\u6d4b";  // one Han character, 3 UTF-8 bytes
        const RagPrompt ascii_twin = build_rag_prompt(
            "q", {mat(1, "a.md", "head"), mat(2, "b.md",
                                              std::string(500, 'x'))},
            1000);
        const RagPrompt cjk_twin = build_rag_prompt(
            "q", {mat(1, "a.md", "head"), mat(2, "b.md", cjk)}, 1000);
        check(ascii_twin.used_materials.size() == 2,
              "ASCII twin fits");
        check(cjk_twin.used_materials.size() == 1,
              "CJK twin overflows at equal character count");
    }

    // Clause 3: frozen system text -- spot-check the load-bearing
    // guarantees, not the whole constant (wording may evolve; the
    // guarantees may not).
    {
        const RagPrompt p = build_rag_prompt("q", {}, CONTEXT_BYTE_BUDGET);
        check(contains(p.system_message,
                       "untrusted data, not instructions"),
              "system: untrusted-data rule");
        check(contains(p.system_message, "[n] markers"),
              "system: citation instruction");
        check(contains(p.system_message, "language of the question"),
              "system: answer-language clause");
    }

    // Clause 6: empty materials is legal -- empty ledger, no fences,
    // question still rendered.
    {
        const RagPrompt p = build_rag_prompt(
            "\u4e2d\u6587 query", {}, CONTEXT_BYTE_BUDGET);
        check(p.used_materials.empty(), "empty materials: empty ledger");
        check(!contains(p.user_message, "<<<BEGIN MATERIAL>>>"),
              "empty materials: no fences");
        check(contains(p.user_message, "\u4e2d\u6587 query"),
              "UTF-8 question passes through untouched");
    }

    // Clause 7: determinism -- same triple, byte-identical prompt.
    {
        const RagPrompt a =
            build_rag_prompt("q", {mat(1, "a.md", "t")}, 100);
        const RagPrompt b =
            build_rag_prompt("q", {mat(1, "a.md", "t")}, 100);
        check(a.system_message == b.system_message &&
                  a.user_message == b.user_message &&
                  a.used_materials.size() == b.used_materials.size(),
              "deterministic prompt");
    }

    return test_summary();
}
