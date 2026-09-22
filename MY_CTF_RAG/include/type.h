// This .h file holds the definitions of all base data structures in the project.
#pragma once
#include <cstddef>
#include <string>
struct Document {
    int id;// Used as the index into vector<Document>.
    std::string path;// Relative path; the document's unique identifier across the project.
    std::string content;// Full text of the document, UTF-8 encoded.
    bool deleted = false;// Tombstone: set to true on soft delete; the id is never recycled.
};
// Document: one loaded raw document. The input of the chunker.

struct Chunk {
    int id;// Global id assigned by the chunker; the primary key of the inverted index and the score table.
    int document_id; // Which document this chunk belongs to; look back into vector<Document> for the path.
    std::string text;// Text of this chunk.
    size_t begin;
    size_t end;
    // Start and end byte offsets into Document::content, half-open [begin, end).
};
// One piece cut from a Document; the minimal unit for indexing and search.


struct SearchResult {
    int chunk_id;    // Id of the hit chunk.
    float score;    // Relevance score; higher means more relevant.
};
// One hit returned by search.
