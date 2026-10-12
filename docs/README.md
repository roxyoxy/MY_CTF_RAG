# Documentation Map

This repo runs on a **docs-first workflow**: contracts and meeting
minutes exist before code, and they outlive it. The doc tree is not
overhead -- it *is* the project's memory. This page is the reading map
for a first-time visitor.

**New here?** Read [`../README.md`](../README.md) first, then come back.
Chinese README: [`README.zh.md`](./README.zh.md).

## The one-file-per-purpose guide

| File | What it is | Read when |
|---|---|---|
| [`../PRINCIPLES.md`](../PRINCIPLES.md) | The constitution: how 3 humans + AI agents collaborate (authorization rules, meeting system, commit discipline) | You want to understand *how* this repo was built, or you are an AI assistant joining the team |
| [`MEETING_LOG.md`](./MEETING_LOG.md) | Append-only meeting minutes: every decision, ruling, and acceptance since day one (166 KB and growing) | You want the full story of any milestone -- search it, don't read it end-to-end |
| [`TASKS.md`](./TASKS.md) | Task dispatch & acceptance ledger: who shipped what, when, with what verification | You want proof-of-work for a milestone |
| [`PARAMS.md`](./PARAMS.md) | The tunable-parameter registry: every `constexpr` knob, its value history, and why it changed | You are tuning retrieval/generation behavior |
| [`m3-plan.md`](./m3-plan.md) | M3 (vector retrieval) construction strategy | You want to see how a milestone gets planned before a line of code exists |
| [`m1-m2-retrospective.md`](./m1-m2-retrospective.md) | M1-M2 retrospective, written for exam review | You want the honest "what went wrong / what we learned" version |
| [`m4-generation-ab.md`](./m4-generation-ab.md) | Generation-model A/B experiment archive (qwen3 1.7b vs 4b-instruct, 9 queries, 5 metrics) | You want to see the experiment discipline: sealed re-runs, byte-identical materials, honest failure modes |
| [`design/`](./design/) | Design docs: deferred features with reasons (`rerank-deferral.md`, `incremental-deferral.md`), specs (`atom_scan-spec.md`), deep-dives (`persist-explained.md`) | You wonder "why didn't they just..." -- the answer is usually here |
| [`assets/`](./assets/) | Demo screenshots & GIFs | You want to see it running |

## Suggested reading order

1. `../README.md` -- what & why (5 min)
2. `../PRINCIPLES.md` -- the unusual collaboration model (10 min)
3. Skim `MEETING_LOG.md` headers (`## [date] role`) -- the project timeline
4. `m1-m2-retrospective.md` -- lessons learned
5. Whatever design doc answers your actual question

## Why keep 166 KB of meeting minutes in an open repo?

Because this project's real subject is **process engineering with AI
collaborators**: every change is authorized, logged, and verifiable.
The minutes are the audit trail that makes the claims in the README
checkable. If that sounds unusual -- that is the point.
