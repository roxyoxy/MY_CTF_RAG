// test_corpus_diff.cpp -- contract tests for corpus_diff.h (M2-3).
#include "check.h"
#include "corpus_diff.h"
#include "type.h"
#include <string>
#include <vector>

static Document makeDoc(int id, const std::string& path,
                        const std::string& content, bool deleted) {
    Document d;
    d.id = id;
    d.path = path;
    d.content = content;
    d.deleted = deleted;
    return d;
}

int main() {
    std::vector<Document> oldc, newc;

    // Two empty corpora -> empty diff.
    const CorpusDiff d1 = diff_corpora(oldc, newc);
    check(d1.added.empty() && d1.removed.empty() && d1.edited.empty(),
          "1 empty corpora -> empty diff");

    // Identical corpora -> empty diff.
    oldc = {makeDoc(0, "a.md", "hello", false),
            makeDoc(1, "b.md", "world", false)};
    const CorpusDiff d2 = diff_corpora(oldc, oldc);
    check(d2.added.empty() && d2.removed.empty() && d2.edited.empty(),
          "2 identical corpora -> empty diff");

    // id is invisible: same paths + contents, renumbered ids.
    newc = {makeDoc(7, "a.md", "hello", false),
            makeDoc(9, "b.md", "world", false)};
    const CorpusDiff d3 = diff_corpora(oldc, newc);
    check(d3.added.empty() && d3.removed.empty() && d3.edited.empty(),
          "3 id differences invisible to the diff");

    // deleted is invisible: same path + content, flags differ. The
    // diff reports corpus facts only; carrying tombstones is the job
    // of inherit_tombstones.
    oldc = {makeDoc(0, "a.md", "x", true)};
    newc = {makeDoc(0, "a.md", "x", false)};
    const CorpusDiff d4 = diff_corpora(oldc, newc);
    check(d4.added.empty() && d4.removed.empty() && d4.edited.empty(),
          "4 deleted flag invisible to the diff");

    // Tail append -> added.
    oldc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "b.md", "2", false)};
    newc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "b.md", "2", false),
            makeDoc(2, "c.md", "3", false)};
    const CorpusDiff d5 = diff_corpora(oldc, newc);
    check(d5.added.size() == 1 && d5.added[0] == "c.md" &&
              d5.removed.empty() && d5.edited.empty(),
          "5 tail append -> added");

    // Middle insert (the ctf-wiki mobile/ scenario): a new path
    // lands between two old ones and every later id shifts. The diff
    // sees one added path; the renumbering is none of its business.
    oldc = {makeDoc(0, "misc/a.md", "1", false),
            makeDoc(1, "pwn/b.md", "2", false)};
    newc = {makeDoc(0, "misc/a.md", "1", false),
            makeDoc(1, "misc/mob.md", "m", false),
            makeDoc(2, "pwn/b.md", "2", false)};
    const CorpusDiff d6 = diff_corpora(oldc, newc);
    check(d6.added.size() == 1 && d6.added[0] == "misc/mob.md" &&
              d6.removed.empty() && d6.edited.empty(),
          "6 middle insert -> one added, id shift irrelevant");

    // Removal -> removed.
    oldc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "b.md", "2", false),
            makeDoc(2, "c.md", "3", false)};
    newc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "c.md", "3", false)};
    const CorpusDiff d7 = diff_corpora(oldc, newc);
    check(d7.removed.size() == 1 && d7.removed[0] == "b.md" &&
              d7.added.empty() && d7.edited.empty(),
          "7 removed path -> removed");

    // Same path, different content -> edited.
    oldc = {makeDoc(0, "a.md", "before", false)};
    newc = {makeDoc(0, "a.md", "after", false)};
    const CorpusDiff d8 = diff_corpora(oldc, newc);
    check(d8.edited.size() == 1 && d8.edited[0] == "a.md" &&
              d8.added.empty() && d8.removed.empty(),
          "8 same path, different content -> edited");

    // All three changes classified in one merge pass.
    oldc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "c.md", "3", false),
            makeDoc(2, "e.md", "5", false)};
    newc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "b.md", "2", false),
            makeDoc(2, "c.md", "33", false),
            makeDoc(3, "d.md", "4", false),
            makeDoc(4, "e.md", "5", false)};
    const CorpusDiff d9 = diff_corpora(oldc, newc);
    check(d9.added.size() == 2 && d9.added[0] == "b.md" &&
              d9.added[1] == "d.md" && d9.edited.size() == 1 &&
              d9.edited[0] == "c.md" && d9.removed.empty(),
          "9 added + edited + unchanged in one merge");

    // Output lists come out ascending (merge order over sorted input).
    oldc = {makeDoc(0, "a.md", "1", false),
            makeDoc(1, "m.md", "2", false),
            makeDoc(2, "z.md", "3", false)};
    newc = {makeDoc(0, "b.md", "1", false),
            makeDoc(1, "c.md", "2", false),
            makeDoc(2, "n.md", "3", false),
            makeDoc(3, "o.md", "4", false)};
    const CorpusDiff d10 = diff_corpora(oldc, newc);
    check(d10.added.size() == 4 && d10.added[0] == "b.md" &&
              d10.added[1] == "c.md" && d10.added[2] == "n.md" &&
              d10.added[3] == "o.md" && d10.removed.size() == 3 &&
              d10.removed[0] == "a.md" && d10.removed[2] == "z.md",
          "10 output lists ascending");

    // M2-4 boundary closure: a tombstone survives an edit of the same
    // path. Inheritance sets deleted, keeps the new content, and
    // leaves other documents alone.
    oldc = {makeDoc(0, "a.md", "old text", true),
            makeDoc(1, "b.md", "b", false)};
    newc = {makeDoc(0, "a.md", "NEW text", false),
            makeDoc(1, "b.md", "b", false)};
    const int n11 = inherit_tombstones(oldc, newc);
    check(n11 == 1 && newc[0].deleted &&
              newc[0].content == "NEW text" && !newc[1].deleted,
          "11 edited tombstone stays deleted, content untouched");

    // No spillover: a tombstoned path missing from the new corpus
    // deletes nothing.
    oldc = {makeDoc(0, "a.md", "x", true),
            makeDoc(1, "b.md", "y", false)};
    newc = {makeDoc(0, "b.md", "y", false),
            makeDoc(1, "c.md", "z", false)};
    const int n12 = inherit_tombstones(oldc, newc);
    check(n12 == 0 && !newc[0].deleted && !newc[1].deleted,
          "12 vanished tombstone path spills onto nothing");

    // No tombstones in the old corpus -> zero flags set.
    oldc = {makeDoc(0, "a.md", "x", false)};
    newc = {makeDoc(0, "a.md", "x", false)};
    check(inherit_tombstones(oldc, newc) == 0 && !newc[0].deleted,
          "13 no tombstones -> 0 inherited");

    // "Flags it set": an already-deleted new document is not counted.
    oldc = {makeDoc(0, "a.md", "x", true)};
    newc = {makeDoc(0, "a.md", "x", true)};
    check(inherit_tombstones(oldc, newc) == 0,
          "14 already-deleted flag not re-counted");

    return test_summary();
}
