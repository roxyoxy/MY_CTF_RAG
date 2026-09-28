// corpus_diff.cpp -- implements corpus_diff.h (M2-3).
//
// One two-pointer merge classifies every path; a second merge walks
// the same order for tombstone inheritance. Both inputs arrive sorted
// by path (loader order / snapshot order), so no internal sort.

#include "corpus_diff.h"

CorpusDiff diff_corpora(const std::vector<Document>& old_docs,
                        const std::vector<Document>& new_docs) {
    CorpusDiff diff;
    size_t i = 0;
    size_t j = 0;
    while (i < old_docs.size() && j < new_docs.size()) {
        if (old_docs[i].path < new_docs[j].path) {
            diff.removed.push_back(old_docs[i].path);
            ++i;
        } else if (new_docs[j].path < old_docs[i].path) {
            diff.added.push_back(new_docs[j].path);
            ++j;
        } else {
            if (old_docs[i].content != new_docs[j].content)
                diff.edited.push_back(new_docs[j].path);
            ++i;
            ++j;
        }
    }
    while (i < old_docs.size()) {
        diff.removed.push_back(old_docs[i].path);
        ++i;
    }
    while (j < new_docs.size()) {
        diff.added.push_back(new_docs[j].path);
        ++j;
    }
    return diff;
}

int inherit_tombstones(const std::vector<Document>& old_docs,
                       std::vector<Document>& new_docs) {
    int inherited = 0;
    size_t i = 0;
    size_t j = 0;
    while (i < old_docs.size() && j < new_docs.size()) {
        if (old_docs[i].path < new_docs[j].path) {
            ++i;
        } else if (new_docs[j].path < old_docs[i].path) {
            ++j;
        } else {
            if (old_docs[i].deleted && !new_docs[j].deleted) {
                new_docs[j].deleted = true;
                ++inherited;
            }
            ++i;
            ++j;
        }
    }
    return inherited;
}
