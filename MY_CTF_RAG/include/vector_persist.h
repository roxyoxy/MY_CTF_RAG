// vector_persist.h -- vector.bin snapshot: dense vectors + identity
// (contract #13).
//
// Role: persist the embedded chunk vectors so a corpus restart does not
// pay full re-embedding (minutes of local inference). index.bin is NOT
// touched: the BM25 snapshot keeps its own M2 contract (persist.h);
// this is a separate snapshot with its own identity and its own failure
// states.
//
// Layout (24-byte header, M2 pattern):
//   magic 8B | FORMAT_VERSION 4B | PIPELINE_VERSION 4B | FNV-1a payload
//   fingerprint 8B
// then payload:
//   identity block (dimension, model_id, embedding_policy), then one
//   record per document. The identity block sits at the payload start,
//   under the fingerprint -- so a tampered identity with a repaired
//   fingerprint is still caught (structure first, identity after).
//
// Ruling note: an independent contract rather than a persist.h
// extension -- different identity model, different invalidation
// semantics, and persist.h stays a frozen M2 deliverable.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

constexpr char VECTOR_MAGIC[8] = { 'M', 'Y', 'R', 'A', 'G', 'V', 'E', 'C' };
constexpr int VECTOR_FORMAT_VERSION = 1;

// The one manual bump knob of the vector pipeline (persist.h
// PIPELINE_VERSION precedent). Bump when the meaning of stored vectors
// changes (layout, normalization policy, reuse semantics). A model
// change does NOT bump this: model identity lives in the identity
// block below and is reported as BAD_MODEL.
constexpr unsigned VECTOR_PIPELINE_VERSION = 1;

// Identity of the vectors inside the snapshot: which coordinate system
// they live in. Changing the model means changing coordinates -- every
// old vector becomes invalid.
struct VectorIdentity {
    std::string model_id;         // e.g. "bge-m3" (embedder.h is the source)
    std::string embedding_policy; // frozen prefix-pair id (embedder.h)
    int dimension = 0;
};

// One record per document, stored in ascending path order (loader
// order). content_hash is the FNV-1a 64 of the document content (the
// manifest precedent): same path + same hash -> vectors reusable as-is.
struct DocVectors {
    std::string path;
    std::uint64_t content_hash = 0;
    std::vector<std::vector<float>> vectors;  // this doc's chunks, chunk order
};

enum class VectorSnapshotStatus {
    OK,          // loaded
    NOT_FOUND,   // no snapshot yet: a normal first-run state, not a failure
    IO_ERROR,    // exists but unreadable
    BAD_MAGIC,   // not a vector.bin
    BAD_FORMAT,  // header / versions / fingerprint / structure broken
    BAD_MODEL,   // structure fine, identity mismatch (other model, other
                 // policy, other dimension) -- "built by a different model";
                 // recovery needs the named model or a full re-embed
};

// Save. Never throws; writes to a .tmp file first, then atomically
// renames (M2 pattern). Returns false on I/O failure -- callers treat
// that as "cache lost", not fatal.
bool vector_save(const std::string& path,
                 const VectorIdentity& identity,
                 const std::vector<DocVectors>& docs);

// Load with the identity check folded in. All-or-nothing, never throws.
//
// 1. expected carries the identity of the provider currently in use;
//    a snapshot built under a different model / policy / dimension
//    returns BAD_MODEL. On OK the snapshot is by definition compatible.
// 2. Check order is fixed and part of this contract (tests assert
//    specific states): magic -> format versions -> fingerprint ->
//    structure walk -> identity. The structure walk validates internal
//    consistency (claimed dimension vs actual vector bytes); identity
//    comparison against expected runs only after the walk.
// 3. On any failure, out is left untouched (the probe stays intact).
// 4. Reuse policy is NOT here. This contract delivers facts (path +
//    hash + vectors); how corpus_diff drives re-embedding of
//    added/edited docs is main's strategy (facts vs policy, the M2
//    corpus_diff rule). A corrupt or stale snapshot is a cache miss:
//    main falls back to a full re-embed.
VectorSnapshotStatus load_vector_snapshot(const std::string& path,
                                          const VectorIdentity& expected,
                                          std::vector<DocVectors>& out);

// FNV-1a 64 of a document's content -- the exact hash this snapshot
// compares through DocVectors::content_hash. Exported so the reuse
// policy in the callers (main / GUI) computes its keys with the same
// algorithm instead of growing private copies: a drifted hash never
// matches, so reuse would quietly stop working while everything still
// runs. Single source of truth (persist.cpp manifest precedent).
std::uint64_t vector_content_hash(const std::string& content);
