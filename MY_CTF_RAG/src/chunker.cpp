// chunker.cpp
// Implements the contract in include/chunker.h.

#include "chunker.h"
#include <cctype>
#include <algorithm>
#include <utility>

namespace {

    // Returns true if the character at position i in text is a word character.
    // Letters, digits, and hyphens unconditionally; '.' and '_' only when
    // sandwiched between letters/digits (sandwich rule).
    static bool isWordChar(const std::string& text, size_t i) {
        char ch = text[i];

        if (std::isalnum(static_cast<unsigned char>(ch))) {
            return true;
        }

        if (ch == '-') {
            return true;
        }

        if (ch == '.' || ch == '_') {
            return i > 0 && i + 1 < text.size() &&
                std::isalnum(static_cast<unsigned char>(text[i - 1])) &&
                std::isalnum(static_cast<unsigned char>(text[i + 1]));
        }

        return false;
    }

} // anonymous namespace

std::vector<Chunk> chunk_documents(const std::vector<Document>& docs)
{
    std::vector<Chunk> chunks;

    // Pass 1: record the byte range of every word in each document.
    for (const auto& doc : docs) {
        std::vector<std::pair<size_t, size_t>> words;
        size_t i = 0;
        size_t len = doc.content.length();

        while (i < len) {
            if (isWordChar(doc.content, i)) {
                size_t start = i;
                while (i < len && isWordChar(doc.content, i)) {
                    i++;
                }
                size_t end = i;
                words.push_back({ start, end });
            }
            else {
                i++;
            }
        }

        // Pass 2: cut into chunks with a sliding window.
        size_t wordCount = words.size();
        if (wordCount == 0) {
            continue;  // no words, skip this document
        }

        size_t s = 0;
        while (true) {
            const size_t e = std::min<size_t>(s + CHUNK_SIZE, wordCount);
            Chunk chunk;                        // fill fields explicitly
            chunk.id = static_cast<int>(chunks.size()); // global, contiguous id
            chunk.document_id = doc.id;
            chunk.begin = words[s].first;       // first byte of word s
            chunk.end = words[e - 1].second;    // last byte of word e-1
            chunk.text = doc.content.substr(chunk.begin, chunk.end - chunk.begin);

            chunks.push_back(chunk);

            if (e == wordCount) {
                break;                          // last chunk reached
            }
            s = e - CHUNK_OVERLAP;              // step back for overlap
        }
    }

    return chunks;
}