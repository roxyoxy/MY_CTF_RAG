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
    const std::string snapshot = "index.bin";

    // Stage 1: build the index once. The snapshot is a cache: restore
    // it when valid, otherwise rebuild and overwrite it (persist.h
    // keeps the trust policy here, in the caller).
    try {
        docs = load_documents("data");

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
    std::cout << "commands: list, del <id>  (anything else is a query)\n";

    // Stage 2: query loop. "list" and "del <id>" are reserved words;
    // every other line is a search query.
    std::string query;
    while (true) {
        std::cout << "query> ";
        if (!std::getline(std::cin, query)) break;
        if (query.empty()) continue;

        if (query == "list") {
            for (size_t i = 0; i < docs.size(); ++i) {
                std::cout << docs[i].id
                    << (docs[i].deleted ? "  [deleted]  " : "  ")
                    << docs[i].content.size() << " bytes  "
                    << docs[i].path << "\n";
            }
            continue;
        }

        if (query.rfind("del ", 0) == 0) {
            const std::string arg = query.substr(4);
            bool digits = !arg.empty();
            for (size_t k = 0; k < arg.size(); ++k)
                if (arg[k] < '0' || arg[k] > '9') digits = false;
            if (!digits) {
                std::cout << "usage: del <doc id>\n";
                continue;
            }
            int id = 0;
            for (size_t k = 0; k < arg.size(); ++k)
                id = id * 10 + (arg[k] - '0');
            if (id >= static_cast<int>(docs.size())) {
                std::cout << "no document with id " << id << "\n";
                continue;
            }
            if (docs[id].deleted) {
                std::cout << docs[id].path << " is already deleted\n";
                continue;
            }
            // Delete = tombstone + immediate rebuild + save, so the
            // snapshot and the running state never disagree.
            docs[id].deleted = true;
            chunks = chunk_documents(docs);
            index = build_index(chunks);
            if (!save(snapshot, docs, chunks, index))
                std::cerr << "Warning: snapshot save failed; "
                    "the in-memory index is still updated.\n";
            std::cout << "deleted " << docs[id].path
                << " (" << chunks.size() << " chunks remain)\n";
            continue;
        }

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