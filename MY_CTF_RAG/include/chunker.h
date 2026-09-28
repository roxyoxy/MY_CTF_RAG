#pragma once

// chunker.h
// Splits documents into chunks.

#include <vector>

#include "type.h"

// atoms per chunk
constexpr int CHUNK_SIZE = 500;

// atoms shared with the previous chunk
constexpr int CHUNK_OVERLAP = 50;

// Splits every document in docs into chunks.
//
// Contract:
// 1. Chunk ids are assigned globally and increase across documents.
//    doc 0 -> ids 0..4, doc 1 -> ids 5..7, and so on.
// 2. chunk_size and overlap are measured in atoms, not bytes.
//    An atom is as defined in atom_scan.h: one ASCII_WORD atom or
//    one CJK_CHAR atom counts as exactly one unit (a Han run of
//    n characters is n atoms, not n-1 bigrams -- the bigram
//    pairing happens later, inside tokenize()). The cut point
//    always falls on an atom boundary.
// 3. Chunk::begin and Chunk::end remain byte offsets into Document::content.
//    Chunk::text equals content.substr(begin, end - begin).
//    Atom-to-byte conversion is done inside the implementation.
// 4. Adjacent chunks overlap by CHUNK_OVERLAP atoms, so a sentence
//    that crosses a cut point appears intact in the next chunk.
// 5. A non-empty document shorter than CHUNK_SIZE produces exactly one chunk.
//    An empty document produces no chunks at all.
std::vector<Chunk> chunk_documents(const std::vector<Document>& docs);
