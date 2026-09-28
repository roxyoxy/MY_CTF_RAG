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


    // Clause 2(b): CJK bigram composition (M2). Inputs are written as
    // \uXXXX escapes so this file stays pure ASCII; with /utf-8 the
    // literals carry the same UTF-8 bytes as the corpus.
    check(tokenize("\u5929\u4E0B") == v({"\u5929\u4E0B"}),
          "13 CJK run of 2 -> one bigram");
    check(tokenize("\u5929\u4E0B\u65E0\u654C") == v({"\u5929\u4E0B", "\u4E0B\u65E0", "\u65E0\u654C"}),
          "14 CJK run of 4 -> three overlapping bigrams");
    check(tokenize("\u5929").empty(),
          "15 isolated Han char emits nothing");
    check(tokenize("\u5929 \u4E0B").empty(),
          "16 space breaks the run (two isolated chars)");
    check(tokenize("\u5929\u3002\u4E0B").empty(),
          "17 full-width stop breaks the run (byte gap)");
    check(tokenize("ROP\u94FE") == v({"rop"}),
          "18 ASCII word then isolated Han char");
    check(tokenize("ROP\u94FE\u5929\u4E0B") == v({"rop", "\u94FE\u5929", "\u5929\u4E0B"}),
          "19 ASCII word then 3-char CJK run -> 2 bigrams");
    check(tokenize("\u5929\xFF\u4E0B").empty(),
          "20 malformed byte breaks the run (byte gap)");
    check(tokenize("\u5929a\u4E0B") == v({"a"}),
          "21 bigram never crosses an ASCII word");
    check(tokenize("\u5929\u4E0B ROP \u65E0\u654C") == v({"\u5929\u4E0B", "rop", "\u65E0\u654C"}),
          "22 mixed runs and words stay independent");

    // Clause 2 mirror judgments (atom_scan.h clause 2, observable
    // through the composition layer): '-' never counts as an alnum
    // neighbor for the sandwich rule.
    check(tokenize("a-.b") == v({"a-", "b"}),
          "23 dot with '-' left neighbor splits");
    check(tokenize("a.-b") == v({"a", "-b"}),
          "24 dot with '-' right neighbor splits");
    check(tokenize("-abc") == v({"-abc"}),
          "25 leading hyphen stays in the word");

    return test_summary();
}
