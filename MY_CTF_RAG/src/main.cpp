// main.cpp
// Assembly line: load, chunk, index, then answer queries in a loop.

#include <exception>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "type.h"
#include "loader.h"
#include "chunker.h"
#include "indexer.h"
#include "persist.h"

// Human-readable name for a snapshot validation failure. The console
// app only prints it; the caller action is always the same (rebuild).
static const char* status_name(SnapshotStatus st) {
    switch (st) {
    case SnapshotStatus::OK:             return "ok";
    case SnapshotStatus::NOT_FOUND:      return "not found (first run)";
    case SnapshotStatus::IO_ERROR:       return "io error";
    case SnapshotStatus::BAD_MAGIC:      return "bad magic";
    case SnapshotStatus::BAD_FORMAT:     return "corrupt";
    case SnapshotStatus::BAD_PIPELINE:   return "pipeline version mismatch";
    case SnapshotStatus::BAD_PARAMS:     return "chunk params mismatch";
    case SnapshotStatus::CORPUS_CHANGED: return "corpus changed";
    }
    return "unknown";
}

int main() {
#ifdef _WIN32
    // Switch the Windows console to UTF-8 so Chinese text shows correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<Document> docs;
    std::vector<Chunk>    chunks;
    InvertedIndex         index;

    // Stage 1: build the index once. The snapshot is a cache: restore
    // it when valid, otherwise rebuild and overwrite it (persist.h
    // keeps the trust policy here, in the caller).
    try {
        docs = load_documents("data");

        const std::string snapshot = "index.bin";
        const SnapshotStatus st = validate(snapshot, docs);
        bool restored = false;
        if (st == SnapshotStatus::OK)
            restored = load(snapshot, docs, chunks, index);

        if (restored) {
            std::cout << "Index restored from snapshot " << snapshot << ".\n";
        } else {
            if (st == SnapshotStatus::OK)
                std::cout << "Snapshot load failed, rebuilding.\n";
            else
                std::cout << "Snapshot unusable ("
                    << status_name(st) << "), rebuilding.\n";
            chunks = chunk_documents(docs);
            index = build_index(chunks);
            if (!save(snapshot, docs, chunks, index))
                std::cerr << "Warning: snapshot save failed; "
                    "the in-memory index is still usable.\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Indexed " << docs.size()
        << " documents, " << chunks.size()
        << " chunks.\n";

    // Stage 2: query loop.
    std::string query;
    while (true) {
        std::cout << "query> ";
        if (!std::getline(std::cin, query)) break;
        if (query.empty()) continue;

        // Stage 3: search and print.
        std::vector<SearchResult> results = search(index, query);

        if (results.empty()) {
            std::cout << "(no results)\n";
            continue;
        }

        for (size_t i = 0; i < results.size(); ++i) {
            const SearchResult& r = results[i];
            const Chunk& c = chunks[r.chunk_id];
            const Document& d = docs[c.document_id];

            std::cout << "[" << (i + 1) << "] "
                << "score=" << r.score << "  "
                << "doc=" << d.path << "\n"
                << "    " << c.text << "\n";
        }
    }

    return 0;
}