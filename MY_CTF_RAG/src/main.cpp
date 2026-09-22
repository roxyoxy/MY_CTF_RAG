// main.cpp
// Assembly line: load, chunk, index, then answer queries in a loop.

#include <iostream>
#include <string>

#include "type.h"
#include "loader.h"
#include "chunker.h"
#include "indexer.h"

int main() {
    // Stage 1: build the index once.
    std::vector<Document> docs = load_documents("data");
    std::vector<Chunk>    chunks = chunk_documents(docs);
    InvertedIndex         index = build_index(chunks);

    std::cout << "Indexed " << docs.size()
        << " documents, " << chunks.size()
        << " chunks.\n";

    // Stage 2: query loop.
    std::string query;
    while (true) {
        std::cout << "query> ";
        if (!std::getline(std::cin, query)) break;   // EOF
        if (query.empty()) continue;                 // skip blank line

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