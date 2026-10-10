// test_vector_index.cpp -- contract tests for vector_index.h (M3 T5).
// Every expectation is a binary-exact value (0, +/-1, +/-0.5, 2.0),
// so plain == is the right comparison throughout -- the golden table
// was designed that way to keep float tolerance disputes out.
#include "check.h"
#include "vector_index.h"

#include <stdexcept>
#include <vector>

namespace {

std::vector<float> vec(std::initializer_list<float> xs) {
    return std::vector<float>(xs);
}

bool order_is(const std::vector<SearchResult>& out,
              std::initializer_list<int> ids) {
    if (out.size() != ids.size())
        return false;
    std::size_t i = 0;
    for (int id : ids) {
        if (out[i].chunk_id != id)
            return false;
        ++i;
    }
    return true;
}

}  // namespace

int main() {
    // Case 1: orthogonal vectors, exact scores 1.0 and 0.0.
    {
        const FlatIndex idx = build_flat_index({vec({1, 0}), vec({0, 1})});
        const std::vector<SearchResult> out =
            search_flat(idx, vec({1, 0}), 2);
        check(order_is(out, {0, 1}), "case1 order [0,1]");
        check(out.size() == 2 && out[0].score == 1.0 &&
                  out[1].score == 0.0,
              "case1 exact scores 1.0 / 0.0");
    }

    // Case 2: identical vectors tie at 1.0, chunk_id ascending.
    {
        const FlatIndex idx = build_flat_index({vec({1, 0}), vec({1, 0})});
        const std::vector<SearchResult> out =
            search_flat(idx, vec({1, 0}), 2);
        check(order_is(out, {0, 1}), "case2 tie-break id ascending");
    }

    // Case 3: top_k = 1 truncates to the winner.
    {
        const FlatIndex idx = build_flat_index({vec({1, 0}), vec({1, 0})});
        const std::vector<SearchResult> out =
            search_flat(idx, vec({1, 0}), 1);
        check(order_is(out, {0}), "case3 k=1 keeps only [0]");
    }

    // Case 4: top_k <= 0 returns empty (M1 guard precedent).
    {
        const FlatIndex idx = build_flat_index({vec({1, 0})});
        check(search_flat(idx, vec({1, 0}), 0).empty(), "case4 k=0 empty");
        check(search_flat(idx, vec({1, 0}), -3).empty(),
              "case4 k=-3 empty");
    }

    // Case 5: top_k beyond N returns everything without breaking.
    {
        const FlatIndex idx = build_flat_index({vec({1, 0}), vec({0, 1})});
        const std::vector<SearchResult> out =
            search_flat(idx, vec({1, 0}), 99);
        check(order_is(out, {0, 1}), "case5 k=99 > N returns all");
    }

    // Case 6: an empty corpus builds an empty legal index; searching
    // it returns empty, never throws.
    {
        const FlatIndex idx = build_flat_index({});
        check(idx.vectors.empty() && idx.dimension == 0,
              "case6 empty build");
        check(search_flat(idx, vec({1, 0}), 5).empty(),
              "case6 search on empty index");
    }

    // Case 7: mixed dimensions at build are a wiring bug -> throw.
    {
        bool threw = false;
        try {
            build_flat_index({vec({1, 0}), vec({1, 0, 0})});
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "case7 mixed dimensions throw");
    }

    // Case 8: query dimension mismatch at search -> throw.
    {
        const FlatIndex idx = build_flat_index({vec({1, 0})});
        bool threw = false;
        try {
            search_flat(idx, vec({1, 0, 0}), 1);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "case8 query dim mismatch throws");
    }

    // Case 9: an unnormalized stored vector scores 2.0, not 1.0 --
    // no re-normalization happens here (clause 3). Feeding
    // non-unit vectors is a caller violation, and this test pins
    // that the index stays literal about it.
    {
        const FlatIndex idx = build_flat_index({vec({2, 0})});
        const std::vector<SearchResult> out =
            search_flat(idx, vec({1, 0}), 1);
        check(out.size() == 1 && out[0].score == 2.0,
              "case9 no re-normalization, dot=2.0");
    }

    // Case 10: three dimensions, 0.5 exact, full ordering.
    {
        const FlatIndex idx = build_flat_index(
            {vec({1, 0, 0}), vec({0, 1, 0}), vec({0.5, 0, 0.5})});
        const std::vector<SearchResult> out =
            search_flat(idx, vec({1, 0, 0}), 3);
        check(order_is(out, {0, 2, 1}), "case10 order [0,2,1]");
        check(out.size() == 3 && out[0].score == 1.0 &&
                  out[1].score == 0.5 && out[2].score == 0.0,
              "case10 scores 1.0 / 0.5 / 0.0");
    }

    return test_summary();
}
