#pragma once

// chunker.h
// Splits documents into chunks.

#include <vector>

#include "type.h"

// words per chunk
constexpr int CHUNK_SIZE = 500;

// words shared with the previous chunk
constexpr int CHUNK_OVERLAP = 50;

// Splits every document in docs into chunks.
//
// Contract:
// 1. Chunk ids are assigned globally and increase across documents.
//    doc 0 -> ids 0..4, doc 1 -> ids 5..7, and so on.
// 2. chunk_size and overlap are measured in words, not bytes.
//    A word is a token as defined by tokenizer.h (clause 2).
//    The cut point always falls on a word boundary.
// 3. Chunk::begin and Chunk::end remain byte offsets into Document::content.
//    Chunk::text equals content.substr(begin, end - begin).
//    Word-to-byte conversion is done inside the implementation.
// 4. Adjacent chunks overlap by CHUNK_OVERLAP words, so a sentence
//    that crosses a cut point appears intact in the next chunk.
// 5. A non-empty document shorter than CHUNK_SIZE produces exactly one chunk.
//    An empty document produces no chunks at all.
std::vector<Chunk> chunk_documents(const std::vector<Document>& docs);