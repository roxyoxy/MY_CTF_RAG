// test_tokenizer.cpp -- contract tests for tokenizer.h (T1).
// Every expectation is derived clause by clause from tokenizer.h.
#include "check.h"
#include "tokenizer.h"
#include <initializer_list>
#include <string>
#include <vector>

static std::vector<std::string> v(std::initializer_list<std::string> xs) {
    return std::vector<std::string>(xs);
}

int main() {
    // Clause 1: lowercases everything.
    check(tokenize("Hello WORLD") == v({"hello", "world"}),
          "1 lowercases everything");

    // Clause 2: tokens are maximal runs of word characters.
    check(tokenize("hello world") == v({"hello", "world"}),
          "2 basic whitespace split");
    check(tokenize("format-string use-after-free") ==
              v({"format-string", "use-after-free"}),
          "3 hyphen is an unconditional word char");
    check(tokenize("127.0.0.1") == v({"127.0.0.1"}),
          "4 dot between digits joins (sandwich)");
    check(tokenize("exploit.py buf_size") == v({"exploit.py", "buf_size"}),
          "5 dot/underscore between alnum joins (sandwich)");
    check(tokenize("__libc_csu_init") == v({"libc_csu_init"}),
          "6 edge underscore splits, inner underscores join");
    check(tokenize("wait... e.g. now") == v({"wait", "e.g", "now"}),
          "7 trailing dot splits, inner dot joins");
    check(tokenize("libc-2.31") == v({"libc-2.31"}),
          "8 hyphen plus sandwich stays one token");
    check(tokenize("hello, WORLD!") == v({"hello", "world"}),
          "9 punctuation separates and is dropped");

    // Clause 4: order of appearance, no dedup, empty -> empty.
    check(tokenize("b a c") == v({"b", "a", "c"}),
          "10 order of appearance preserved");
    check(tokenize("a a b") == v({"a", "a", "b"}),
          "11 duplicates kept (no dedup)");
    check(tokenize("").empty() && tokenize("... !!! ??? ").empty(),
          "12 empty / separator-only input -> empty vector");

    return test_summary();
}
