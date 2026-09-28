// chunker.cpp
// Implements the contract in include/chunker.h.

#include "chunker.h"

#include <algorithm>

#include "atom_scan.h"

std::vector<Chunk> chunk_documents(const std::vector<Document>& docs) {
    std::vector<Chunk> chunks;

    for (const auto& doc : docs) {
        if (doc.deleted) {
            continue;  // tombstone: contributes no chunks (clause 6)
        }

        // Pass 1: one call to the single source of truth for lexical units.
        // The atom table carries three things at once: count (chunk unit),
        // begin/end (byte span), and kind (which we do not need here --
        // chunker counts all atoms uniformly, ASCII and CJK alike).
        const std::vector<Atom> atoms = scan_atoms(doc.content);
        const size_t atom_count = atoms.size();

        if (atom_count == 0) {
            continue;  // empty or separator-only document
        }

        // Pass 2: sliding window over atoms, cutting on atom boundaries.
        size_t s = 0;
        while (true) {
            const size_t e = std::min<size_t>(s + CHUNK_SIZE, atom_count);

            Chunk chunk;                                // fill fields explicitly
            chunk.id = static_cast<int>(chunks.size()); // global, contiguous id
            chunk.document_id = doc.id;
            chunk.begin = atoms[s].begin;               // byte offset of first atom
            chunk.end = atoms[e - 1].end;               // byte offset past last atom
            chunk.text = doc.content.substr(chunk.begin, chunk.end - chunk.begin);

            chunks.push_back(chunk);

            if (e == atom_count) {
                break;                                  // last chunk reached
            }
            s = e - CHUNK_OVERLAP;                      // step back for overlap
        }
    }

    return chunks;
}