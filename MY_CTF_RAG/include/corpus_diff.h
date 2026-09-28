// corpus_diff.h
// Eighth contract: corpus change detection. Given two corpus
// generations, report what changed (facts), and carry tombstones
// across a rebuild so a deletion survives corpus edits.

#pragma once

#include <string>
#include <vector>

#include "type.h"

// Facts about what changed between two corpus generations.
// Every list holds relative paths, sorted ascending.
struct CorpusDiff {
    std::vector<std::string> added;    // present only in the new corpus
    std::vector<std::string> removed;  // present only in the old corpus
    std::vector<std::string> edited;   // same path, different content
};

// 1. diff_corpora walks two sorted corpora with two pointers, one
//    pass, O(n + m). Every path is classified exactly once: only in
//    new -> added, only in old -> removed, in both -> edited when the
//    content bytes differ, unchanged otherwise.
// 2. Identity is the path; an edit is a content change. The id and
//    deleted fields are invisible to the diff -- the same purity rule
//    validate() applies to its manifest comparison.
// 3. Preconditions: both inputs are sorted ascending by path, exactly
//    as load_documents produces and as a snapshot restore returns.
//    Unsorted input gives unspecified results (no internal sort).
// 4. The diff is facts, not policy. What the caller does with a diff
//    (rebuild, report, ignore) is the caller's business.
// 5. inherit_tombstones sets deleted = true on every new document
//    whose path was deleted in the old corpus and returns the number
//    of flags it set. Content is not consulted: editing a deleted
//    document does not resurrect it. The escape hatch is physical:
//    remove the file, then add it back.
// 6. Both functions do no I/O and throw nothing.

CorpusDiff diff_corpora(const std::vector<Document>& old_docs,
                        const std::vector<Document>& new_docs);

int inherit_tombstones(const std::vector<Document>& old_docs,
                       std::vector<Document>& new_docs);
