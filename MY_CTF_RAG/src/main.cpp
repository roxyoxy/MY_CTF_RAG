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

int main() {
#ifdef _WIN32
    // Switch the Windows console to UTF-8 so Chinese text shows correctly.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<Document> docs;
    std::vector<Chunk>    chunks;
    InvertedIndex         index;

    // Stage 1: build the index once.
    try {
        docs = load_documents("data");
        chunks = chunk_documents(docs);
        index = build_index(chunks);
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