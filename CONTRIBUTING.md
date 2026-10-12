# Contributing

Thanks for your interest in MY_RAG! This repo follows a
**docs-first, contract-first** workflow that is a bit unusual -- reading
[this section](./PRINCIPLES.md) first will save you a round trip.

## Environment

- Windows 10/11 + Visual Studio 2022 (Desktop development with C++)
- Qt 6 (only for the optional GUI, `MY_CTF_RAG_GUI`)
- [Ollama](https://ollama.com/download) (optional; the app degrades
  gracefully to BM25-only retrieval without it)

## Build

Open `MY_CTF_RAG/MY_CTF_RAG.sln` in VS2022 and build
(Debug or Release, x64), or from a developer prompt:

```bat
cd MY_CTF_RAG
msbuild MY_CTF_RAG.sln /p:Configuration=Release /p:Platform=x64
```

## The regression gate

Before any commit, run the full suite (243 cases, ~1 min) and keep it green:

```bat
tests\run_tests.bat
```

Exit code 0 and `ALL_TESTS_GREEN` means you are clear.

## Hard rules (from PRINCIPLES.md)

1. **Pure-ASCII source.** All `.h/.cpp` comments and literals are ASCII
   English only. Dashes are `--`, never `—`. (The one documented exception:
   Chinese UI literals in `gui/mainwindow.cpp`.)
2. **Contracts live in `include/`.** A module's public shape is its header;
   implementation files only fulfill it. Changing a contract means changing
   the header first and recording why.
3. **Tunable parameters** are `constexpr` in the module header and registered
   in `docs/PARAMS.md`.
4. **Warnings are errors in spirit**: `/W4` must stay clean in both
   compilers used locally.
5. **Third-party code is include-only**: `third_party/` is never edited.

## Suggested workflow

1. Read `README.md`, then `PRINCIPLES.md`, then skim
   `docs/MEETING_LOG.md` (the project's shared memory).
2. Open an issue describing what you want to change and why.
3. Small, focused PRs. Every behavior change should carry a test in
   `MY_CTF_RAG/tests/`.
4. `run_tests.bat` green + build clean = ready for review.
