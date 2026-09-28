#pragma once

// persist.h -- snapshot persistence for the derived index (7th contract)
//
// Philosophy: the snapshot is a CACHE, not a database. data/ is the
// single source of truth; the snapshot stores "work already done" and
// can be deleted and rebuilt at any time. Every bad validate() state
// means the same thing to the caller: cache miss, rebuild.
//
// Division of labor: persist is DUMB. It only moves data between memory
// and the snapshot file. The POLICY (trust it or rebuild?) lives in
// main:
//
//     docs = load_documents(data_dir);
//     st   = validate(idx_path, docs);
//     if (st == SnapshotStatus::OK && load(idx_path, docs, chunks, index))
//         ;  // cache hit
//     else {
//         chunks = chunk_documents(docs);
//         index  = build_index(chunks);
//         save(idx_path, docs, chunks, index);  // failure = warning only
//     }
//
// persist never calls loader/chunker/indexer and never walks a
// directory. The snapshot path is chosen by the caller; persist does
// not know or care where the file lives.

#include <cstdint>
#include <string>
#include <vector>

#include "indexer.h"
#include "type.h"

// Disk format version. Bump when the on-disk layout changes (field
// added / removed / reordered, widths, header). Snapshots stamped with
// a different FORMAT_VERSION are rejected as BAD_FORMAT and rebuilt;
// old files are never migrated, only discarded.
constexpr uint32_t FORMAT_VERSION = 1;

// Pipeline version: versions the SEMANTICS of the whole derivation
// chain (chunk -> tokenize -> build_index), not the disk layout.
// This is the only knob in the project that a human must remember
// to bump by hand. Bump when ANY of these changes:
//   - Han character ranges in atom_scan.h
//   - bigram policy in tokenize() (e.g. bigram -> trigram)
//   - sandwich rule / word-internal character rules
//   - what counts as one atom (the chunker counting unit)
//   - index scoring semantics (M3+)
// Do NOT bump for:
//   - CHUNK_SIZE / CHUNK_OVERLAP value changes (stored on disk and
//     checked at validate time -> BAD_PARAMS)
//   - BM25 k1 / b (query-time parameters, not part of the index)
// Forgetting to bump = silently stale index: every check passes and
// queries run against OLD semantics with no error at all.
constexpr uint32_t PIPELINE_VERSION = 1;

// Outcome of validate(). Seven bad states, one good. Every bad state
// maps to the same caller action (rebuild) but tells the user a
// different story; tests assert exact values.
enum class SnapshotStatus {
    OK,             // all checks passed, snapshot is trustworthy
    NOT_FOUND,      // snapshot file does not exist (first run: normal)
    IO_ERROR,       // file exists but cannot be read (permission, lock)
    BAD_MAGIC,      // first 8 bytes are not our magic: not our snapshot
    BAD_FORMAT,     // FORMAT_VERSION unsupported, payload fingerprint
                    // mismatch, or structure truncated / corrupt
    BAD_PIPELINE,   // PIPELINE_VERSION differs (semantics changed)
    BAD_PARAMS,     // CHUNK_SIZE / CHUNK_OVERLAP differ from build time
    CORPUS_CHANGED  // manifest does not match the current corpus bytes
};

// save: serialize docs + chunks + index into a custom binary snapshot.
//   1. What goes into the snapshot, complete list. Header: magic,
//      FORMAT_VERSION, PIPELINE_VERSION, and a payload fingerprint
//      (computed over every payload byte; it catches on-disk
//      corruption that stays parseable). Payload: CHUNK_SIZE,
//      CHUNK_OVERLAP, documents {id, path, content, deleted},
//      chunks {id, document_id, begin, end} WITHOUT text, postings,
//      chunk_lengths, and a manifest of {path, content fingerprint}
//      per document. This list is exactly the set of things
//      validate() checks; nothing else is stored.
//   2. Cheap derivations are NOT stored: Chunk::text is restored as
//      content.substr(begin, end - begin) (chunker.h clause 3);
//      avg_chunk_length is recomputed from chunk_lengths, and an empty
//      chunk table yields 0.0 (indexer.h clause 4 guard).
//   3. Deterministic serialization: postings are written in term
//      byte-lexicographic order and the manifest in path order; the
//      file contains no timestamps and no random bytes. The same
//      logical state always produces the exact same file bytes.
//   4. Atomic replace: write a temporary file, close it, then rename
//      over the target. A crash mid-save never destroys the previous
//      snapshot.
//   5. Never throws. Returns false on any failure (the in-memory
//      index is still valid; a failed save is only a lost cache).
bool save(const std::string& path,
    const std::vector<Document>& docs,
    const std::vector<Chunk>& chunks,
    const InvertedIndex& index);

// validate: can the snapshot at `path` be trusted for THIS corpus?
// `docs` is the freshly loaded corpus (the reality probe); validate
// hashes content in memory and never touches the file system beyond
// the snapshot itself. Checks run in a FIXED order and the first
// failing check decides the return value, so tests can assert exact
// outcomes:
//   1. file exists?                 -> NOT_FOUND
//   2. readable?                    -> IO_ERROR
//   3. magic matches?               -> BAD_MAGIC
//   4. format + payload fingerprint -> BAD_FORMAT
//   5. pipeline version?            -> BAD_PIPELINE
//   6. chunk params?                -> BAD_PARAMS
//   7. manifest vs docs?            -> CORPUS_CHANGED
//   all pass                        -> OK
// Purity: the manifest compares PATH + CONTENT bytes only, never
// id / deleted. It answers "is the input corpus the same batch of
// bytes?", not "is the runtime state the same?".
// Never throws.
SnapshotStatus validate(const std::string& path,
    const std::vector<Document>& docs);

// load: restore docs + chunks + index from a trusted snapshot.
//   1. All-or-nothing: on success the three outputs are delivered
//      in full and READY TO USE (chunk.text restored, avgdl
//      recomputed); on failure all three outputs are left
//      completely untouched: docs keeps whatever the caller
//      passed in (the loader probe), chunks and index keep their
//      previous values, so the fallback path
//      (chunk_documents(docs) + build_index) always has a valid
//      docs to work from. A half-loaded state is never visible.
//   2. docs is an OUTPUT: on success it holds the snapshot's runtime
//      state (path / content / deleted), which may differ from the
//      loader probe once deletion exists (M2-4). The probe's job ends
//      when validate returns OK.
//   3. Re-checks the header itself (magic / versions / params) as
//      defense-in-depth for callers that skip validate (e.g. a GUI).
//   4. Never throws. Returns false on any failure; the caller falls
//      back to a fresh build.
bool load(const std::string& path,
    std::vector<Document>& docs,
    std::vector<Chunk>& chunks,
    InvertedIndex& index);