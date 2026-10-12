# MY_RAG — Offline CTF RAG with Self-Built Retrieval Algorithms

**An offline Retrieval-Augmented Generation system for CTF players: feed it your writeups, ask it about a challenge, get a grounded answer with clickable citations — no cloud, no telemetry, everything runs on your machine.**

> 中文完整版 README：[`docs/README.zh.md`](./docs/README.zh.md)

[![CI](https://github.com/roxyoxy/MY_CTF_RAG/actions/workflows/ci.yml/badge.svg)](https://github.com/roxyoxy/MY_CTF_RAG/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
[![tests](https://img.shields.io/badge/tests-243%20passing-brightgreen)](./MY_CTF_RAG/tests)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](./MY_CTF_RAG/MY_CTF_RAG.sln)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)](./CONTRIBUTING.md)

<!-- Demo GIF slot: record during the demo session, save as docs/assets/demo.gif
![demo](./docs/assets/demo.gif) -->

```
query: "ret2libc, NX on, libc 2.31"
   ↓  hybrid retrieval over YOUR writeup corpus
   ↓  BM25 (self-built) ⊕ dense vectors (local embedding model) → RRF
   ↓  context assembly with citation ledger
   ↓  local LLM (qwen3, CPU)
answer: exploitation strategy with [1][2][3] citations,
        each [n] clickable → jumps to the exact source chunk
```

## Why this exists

1. **Real competition constraint.** Network-isolated CTFs cannot use cloud
   AI. This system is fully local: retrieval, embedding, and generation
   all run on your machine (CPU-only is a first-class target).
2. **Algorithms as the subject.** Built for an algorithm-design course:
   BM25, Chinese bigram tokenization, exact Flat search, HNSW, and RRF
   fusion are hand-written and explainable. Embedding and LLM are treated
   as swappable infrastructure — the boundary is deliberate and documented.
3. **Security-flavored RAG.** From a cybersecurity program: citation
   sanity checks (out-of-range `[n]` flagged red), grounded-only answering,
   and a strict untrusted-data rule in the prompt contract.

## What is hand-built vs. imported

| Layer | Approach |
|---|---|
| Tokenizer (EN + CJK bigram) / BM25 / chunker / persistence / incremental corpus diff / tombstone delete | **Self-built** (M1/M2) |
| Flat exact vector search / **self-built HNSW** / RRF hybrid fusion | **Self-built** (M3) |
| Context builder with byte-honest budget + citation ledger | **Self-built** (M4) |
| Embedding model & LLM (qwen3 via Ollama **or** llama.cpp) | Open-source models, isolated behind provider interfaces |
| cpp-httplib / nlohmann-json (include-only, `third_party/`) | Vendored, MIT |

Swap runtimes without touching the pipeline: the same model runs through
Ollama (prototype) or llama.cpp (delivery) — same machine, same quant,
**llama.cpp answers ~2x faster** (3.5-4.2s vs 8.4s).

## Architecture

```
        data/ — 58 real CTF writeups (markdown)
           │ loader + corpus_diff (incremental add/remove/edit)
           ▼
        chunker (500 words / 50 overlap)
           │
   ┌───────┴────────────┐
   ▼                    ▼
 BM25 route        dense route
 tokenizer +       embedding model (local HTTP)
 inverted index         │
   │                    ▼
   │              Flat (exact) | HNSW (self-built)
   │                    │
   └────────┬───────────┘
            ▼
      RRF fusion (k = 60)
            ▼
   context builder — byte-honest UTF-8 budget,
   whole-material in/out, citation ledger [n]
            ▼
   local LLM (qwen3:4b-instruct, CPU)
            ▼
   grounded answer, [n] citations clickable in the Qt GUI
```

**Degradation is a feature**: embedding service down → BM25-only answers;
LLM down → retrieval-only results. Missing models never block the app.

## Highlights (measured, reproducible)

- **Self-built HNSW**: Recall@10 = **1.000** vs exact Flat on clustered
  synthetic data; deterministic tie-breaks, seeded builds.
- **The hybrid golden case**: a Chinese synonym query where BM25 recalls
  *nothing* and the dense route rescues it (`bm25=- vec=0.53`) — the
  textbook argument for hybrid fusion, caught live.
- **Warm-start vector cache**: per-doc `path + content-hash` reuse keys —
  145 vectors reload in **0.01s** (vs 20s cold embed); editing one doc
  re-embeds only that doc.
- **243 regression cases** across 15 units, `/W4` clean, run by one
  script: `tests\run_tests.bat`.
- **Citation integrity**: every `[n]` maps to a ledger entry;
  out-of-range citations render red and get counted (GUI).

## Quick start

Prerequisites: **VS2022** (Desktop C++), **Qt 6** (GUI only), **Ollama**
(optional — graceful degradation without it).

```bat
git clone https://github.com/roxyoxy/MY_CTF_RAG.git
:: build: open MY_CTF_RAG\MY_CTF_RAG.sln, build x64 (Debug or Release)
:: or from a developer prompt:
cd MY_CTF_RAG && msbuild MY_CTF_RAG.sln /p:Configuration=Release /p:Platform=x64
```

Pull the local models (~4.5 GB total, all offline afterwards):

```bat
ollama pull qwen3-embedding:0.6b   :: retrieval vectors (A/B-tested winner)
ollama pull qwen3:4b-instruct      :: generation (NOT qwen3:4b — that tag
                                     :: now points to a thinking variant)
```

Run the console app (`MY_CTF_RAG.exe`): it builds the index on first
launch (cached in `index.bin` / `vector.bin` afterwards), then gives you
a `query>` prompt — Chinese and English both work. The Qt GUI
(`MY_CTF_RAG_GUI.exe`) adds corpus management, a results table with
route visibility (`bm25+vec` / `bm25` / `vec`), and the chat page with
clickable citations.

Prefer llama.cpp as the runtime? Launch two `llama-server` instances
(OpenAI-compatible endpoints) and the same pipeline works through the
llama.cpp providers — commands in [`docs/README.zh.md`](./docs/README.zh.md#4-快速开始).

### The corpus

58 real post-competition writeups (22 campus-CTF `npusec` EN/CN pairs +
36 ISCC 2026 solutions, six categories incl. mobile). The corpus is
plain markdown by category — drop in your own writeups and the system
re-indexes incrementally (`added/removed/edited` diff on startup).

## Project structure

```
MY_RAG/                     docs-first repo root (you are here)
├── README.md               English README
├── LICENSE / CONTRIBUTING.md
├── PRINCIPLES.md           the constitution: humans + AI agents collaboration
├── .github/workflows/      CI: Windows build + 243-test regression
├── docs/                   README.md index · README.zh.md · MEETING_LOG (166KB
│                           of real decisions) · TASKS · PARAMS · plans · design/
├── scripts/                meeting-query tool · chunk dump · A/B eval harness
└── MY_CTF_RAG/             VS2022 solution
    ├── include/            15 contracts (the .h files ARE the API)
    ├── src/                implementations + main.cpp assembly
    ├── tests/              243 cases, run_tests.bat one-shot
    ├── gui/                Qt6 front-end (no-moc house rule, worker threads)
    ├── data/               the corpus
    └── third_party/        httplib + nlohmann/json, include-only, MIT
```

The include/src split is deliberate: headers are contracts, sources are
fulfillment. Read `include/` and you know the whole system.

## The unusual part: an auditable AI-collaboration process

This repo was built by 3 humans + AI agents under a written constitution
(`PRINCIPLES.md`): every modification authorized per-instance, every
decision logged in append-only meeting minutes, every claim checkable.
Start at [`docs/README.md`](./docs/README.md) for the map.

## Status

M0-M4 complete (retrieval, persistence, GUI, generation, dual runtimes).
M5 — comparative experiments (BM25 vs Flat vs HNSW vs Hybrid, two-layer
evaluation) — in progress. Test corpus: 58 docs / 145 chunks.

## Contributing & License

See [`CONTRIBUTING.md`](./CONTRIBUTING.md). **MIT** — see
[LICENSE](./LICENSE). Vendored dependencies keep their own MIT notices
in `third_party/`.
