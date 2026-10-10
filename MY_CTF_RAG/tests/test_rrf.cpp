// test_rrf.cpp -- contract tests for rrf.h (M3 T6).
// Golden expectations use the same fractions the implementation
// accumulates (1/(RRF_K + rank)) and are then rounded to float the
// same way the implementation stores them, so comparisons stay
// bit-exact without spelling out decimals. Input scores inside the
// ranked lists are ignored by design -- routes carry rank only --
// so the helper fills them with an arbitrary sentinel.
#include "check.h"
#include "rrf.h"

#include <vector>

namespace {

std::vector<SearchResult> route(const std::vector<int>& xs) {
    std::vector<SearchResult> out;
    out.reserve(xs.size());
    for (int id : xs)
        out.push_back(SearchResult{id, -1.0f});
    return out;
}

}  // namespace

int main() {
    // Case 1: double tie. doc0 = 1/61 + 1/62 and doc1 = 1/62 + 1/61
    // are bit-equal doubles (IEEE addition is commutative) and stay
    // equal after the float rounding at storage, so both pairs fall
    // through to the chunk_id tie-break; doc2 and doc3 both hold a
    // lone 1/63. Expected order [0, 1, 2, 3].
    {
        const std::vector<SearchResult> out =
            rrf_fuse({route({0, 1, 2}), route({1, 0, 3})});
        check(out.size() == 4, "case1 size");
        bool order = out.size() == 4 && out[0].chunk_id == 0 &&
                     out[1].chunk_id == 1 && out[2].chunk_id == 2 &&
                     out[3].chunk_id == 3;
        check(order, "case1 double tie order [0,1,2,3]");
        check(out[0].score ==
                  static_cast<float>(1.0 / (RRF_K + 1) +
                                     1.0 / (RRF_K + 2)),
              "case1 doc0 fused score");
        check(out[1].score ==
                  static_cast<float>(1.0 / (RRF_K + 2) +
                                     1.0 / (RRF_K + 1)),
              "case1 doc1 fused score (bit-equal by commutativity)");
        check(out[2].score == static_cast<float>(1.0 / (RRF_K + 3)) &&
                  out[3].score == static_cast<float>(1.0 / (RRF_K + 3)),
              "case1 doc2/doc3 lone 1/63");
    }

    // Case 2: a single non-empty list degenerates to a re-scored
    // passthrough (clause 5).
    {
        const std::vector<SearchResult> out = rrf_fuse({route({5, 7})});
        check(out.size() == 2 && out[0].chunk_id == 5 &&
                  out[1].chunk_id == 7,
              "case2 passthrough order");
        check(out[0].score == static_cast<float>(1.0 / (RRF_K + 1)) &&
                  out[1].score == static_cast<float>(1.0 / (RRF_K + 2)),
              "case2 rescored ranks");
    }

    // Case 3: no lists, and lists that are all empty, both fuse to
    // nothing (clause 4).
    {
        check(rrf_fuse({}).empty(), "case3 no lists");
        check(rrf_fuse({{}, {}}).empty(), "case3 all lists empty");
    }

    // Case 4: a repeated chunk_id counts once per list at its first
    // (best) position, and the entry after it keeps its original
    // offset -- doc9 stays at rank 3, it does not shift up to 2.
    {
        const std::vector<SearchResult> out = rrf_fuse({route({4, 4, 9})});
        check(out.size() == 2 && out[0].chunk_id == 4 &&
                  out[1].chunk_id == 9,
              "case4 order");
        check(out[0].score == static_cast<float>(1.0 / (RRF_K + 1)),
              "case4 duplicate counted once at first rank");
        check(out[1].score == static_cast<float>(1.0 / (RRF_K + 3)),
              "case4 later doc not shifted up");
    }

    // Case 5: three routes, doc0 appears in two of them.
    {
        const std::vector<SearchResult> out =
            rrf_fuse({route({0}), route({0}), route({1})});
        check(out.size() == 2 && out[0].chunk_id == 0 &&
                  out[1].chunk_id == 1,
              "case5 order [0,1]");
        check(out[0].score ==
                  static_cast<float>(1.0 / (RRF_K + 1) +
                                     1.0 / (RRF_K + 1)),
              "case5 doc0 accumulates across routes");
    }

    // Case 6: disjoint routes. doc0 and doc2 tie at 1/61, doc1 and
    // doc3 tie at 1/62; ties break by id across routes too.
    {
        const std::vector<SearchResult> out =
            rrf_fuse({route({0, 1}), route({2, 3})});
        check(out.size() == 4 && out[0].chunk_id == 0 &&
                  out[1].chunk_id == 2 && out[2].chunk_id == 1 &&
                  out[3].chunk_id == 3,
              "case6 interleaved tie order [0,2,1,3]");
        check(out[0].score == static_cast<float>(1.0 / (RRF_K + 1)) &&
                  out[2].score == static_cast<float>(1.0 / (RRF_K + 2)),
              "case6 scores");
    }

    return test_summary();
}
