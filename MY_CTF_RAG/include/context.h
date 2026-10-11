// context.h -- retrieval-to-LLM context assembly (contract #15).
//
// Role: takes the query and the ranked retrieval results (already
// fused by RRF in the caller) and produces the exact two-message prompt
// the generator sends -- plus the citation ledger that maps [n] markers
// back to the chunks actually included. The prompt template lives here
// and nowhere else: main and the GUI must not keep private copies of
// it, for the same reason vector_content_hash() was exported -- one
// source, zero drift between frontends.
//
// The ledger is the whole point of this contract's shape: a caller
// that only got one big string would have to re-parse the prompt to
// learn what [3] means. used_materials[0] IS [1], by construction.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Registered in docs/PARAMS.md. Bytes, not characters: std::string's
// size() counts UTF-8 bytes, and this project has been byte-honest
// since the M1 chunker. M5 sweeps 8K/16K/32K.
constexpr std::size_t CONTEXT_BYTE_BUDGET = 16384;

// One retrievable unit, filled by the caller from the fused results.
struct RagMaterial {
    int chunk_id;             // stable chunk id (chunk table index)
    std::string source;       // document path or other stable label
    std::string text;         // raw chunk text, UTF-8
};

struct RagPrompt {
    std::string system_message;
    std::string user_message;

    // Exactly the materials actually included after budget trimming,
    // in citation order: used_materials[0] rendered as [1].
    std::vector<RagMaterial> used_materials;
};

// Contract clauses:
// 1. ranked_materials arrives in rank order (the caller passes its
//    fused top results as-is). Materials enter the prompt strictly in
//    that order, numbered [1], [2], ... in arrival order.
// 2. byte_budget bounds the rendered material blocks (header line +
//    fences + text per material), not the query or template
//    scaffolding. Materials are added whole, walking rank order; the
//    walk stops before the first whole material that would overflow
//    the budget. A chunk is never cut mid-way. Degenerate guard: the
//    top-ranked material is always included, even if it alone exceeds
//    the budget -- non-empty input never yields an empty context.
// 3. system_message is a frozen constant owned here (English, pure
//    ASCII): retrieved materials are untrusted data, never
//    instructions; answer only from the materials; say so when they
//    are insufficient; cite with [n]; answer in the language of the
//    question. Callers never write their own system text.
// 4. user_message shape is frozen: the question first, then numbered
//    material blocks:
//        [n]
//        Source: <source>
//        <<<BEGIN MATERIAL>>>
//        <text>
//        <<<END MATERIAL>>>
//    The [n] numbering matches used_materials exactly.
// 5. Injection mitigation positioning: the untrusted-data rule lives
//    in the SYSTEM message and materials are fence-wrapped, so the
//    defense and the attacker's text never share one layer
//    (instruction/data separation by construction). Scope is
//    mitigation, not isolation -- the honest defense-demo wording.
// 6. An empty ranked_materials vector is legal: the returned prompt
//    carries zero material blocks and an empty ledger, and the system
//    text already tells the model to say the material is insufficient.
//    No throw.
// 7. Pure function: no I/O, no globals, no clock; the same
//    (query, materials, budget) triple always yields a byte-identical
//    prompt.
RagPrompt build_rag_prompt(const std::string& query,
                           const std::vector<RagMaterial>& ranked_materials,
                           std::size_t byte_budget);
