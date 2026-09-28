// test_atom_scan.cpp -- contract tests for atom_scan.h (M2).
// Every expectation is derived clause by clause from atom_scan.h.
// Chinese inputs are written as \uXXXX escapes so this file stays
// pure ASCII; with /utf-8 the literals carry the same UTF-8 bytes
// as the corpus.
#include "atom_scan.h"
#include "check.h"
#include <string>
#include <vector>

static bool atom_eq(const Atom& a, AtomKind k, size_t b, size_t e) {
    return a.kind == k && a.begin == b && a.end == e;
}

int main() {
    // Clause: nothing but words and Han characters becomes an atom.
    check(scan_atoms("").empty() && scan_atoms("... !!! ??? ").empty(),
          "1 empty / separator-only input -> no atoms");

    // Clause 2: maximal run of [A-Za-z0-9-], byte offsets [begin, end).
    {
        const auto a = scan_atoms("hello world");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 5) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 6, 11),
              "2 two words with exact spans");
    }
    {
        const auto a = scan_atoms("format-string use-after-free");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 13) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 14, 28),
              "3 hyphen unconditionally word-internal");
    }
    {
        // Hyphen at the leading/trailing edge and standalone.
        const auto a = scan_atoms("-abc abc- -");
        check(a.size() == 3 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 4) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 5, 9) &&
                  atom_eq(a[2], AtomKind::ASCII_WORD, 10, 11),
              "4 hyphen edges and standalone hyphen stay whole");
    }

    // Clause 2 sandwich: '.'/'_' need alnum on BOTH sides.
    check(scan_atoms("127.0.0.1").size() == 1,
          "5 dots between digits join");
    {
        const auto a = scan_atoms("done. wait...");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 4) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 6, 10),
              "6 trailing dot and ellipsis split");
    }
    {
        const auto a = scan_atoms("e.g. now");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 3) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 5, 8),
              "7 first dot of e.g. joins, trailing one does not");
    }
    {
        const auto a = scan_atoms("__libc_csu_init");
        check(a.size() == 1 && atom_eq(a[0], AtomKind::ASCII_WORD, 2, 15),
              "8 leading underscores stripped, inner ones join");
    }

    // Mirror judgments: '-' never counts as an alnum neighbor.
    {
        const auto a = scan_atoms("a-.b");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 2) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 3, 4),
              "9 dot with '-' left neighbor splits");
    }
    {
        const auto a = scan_atoms("a.-b");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 1) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 2, 4),
              "10 dot with '-' right neighbor splits");
    }

    // Clause 3: Han code point ranges, one CJK_CHAR atom per character.
    {
        const auto a = scan_atoms("\u5929\u4E0B");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::CJK_CHAR, 0, 3) &&
                  atom_eq(a[1], AtomKind::CJK_CHAR, 3, 6),
              "11 two Han chars, byte-adjacent spans");
    }
    {
        // U+3400, U+4E00, U+9FFF, U+4DBF: both range edges pinned.
        const auto a = scan_atoms("\u3400\u4E00\u9FFF\u4DBF");
        check(a.size() == 4 && atom_eq(a[0], AtomKind::CJK_CHAR, 0, 3) &&
                  atom_eq(a[1], AtomKind::CJK_CHAR, 3, 6) &&
                  atom_eq(a[2], AtomKind::CJK_CHAR, 6, 9) &&
                  atom_eq(a[3], AtomKind::CJK_CHAR, 9, 12),
              "12 all four Han range edges accepted");
    }
    {
        // U+3002 full-width stop: valid UTF-8, outside Han -> separator.
        const auto a = scan_atoms("\u5929\u3002\u4E0B");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::CJK_CHAR, 0, 3) &&
                  atom_eq(a[1], AtomKind::CJK_CHAR, 6, 9),
              "13 full-width stop breaks the run");
    }
    {
        // U+4DC0 (Yijing hexagram): lead byte E4 would tempt a range
        // guess; the code point check must reject it. 3 bytes consumed.
        const auto a = scan_atoms("\u4DC0" "a");
        check(a.size() == 1 && atom_eq(a[0], AtomKind::ASCII_WORD, 3, 4),
              "14 U+4DC0 rejected by code point, not lead byte");
    }
    {
        // U+1F600 (astral, valid 4-byte sequence): consumed whole.
        const auto a = scan_atoms("\U0001F600" "a");
        check(a.size() == 1 && atom_eq(a[0], AtomKind::ASCII_WORD, 4, 5),
              "15 astral code point skipped as one unit");
    }

    // Clause: malformed UTF-8 -> skip one byte, keep scanning.
    {
        const auto a = scan_atoms("a\xFF" "b");
        check(a.size() == 2 && atom_eq(a[0], AtomKind::ASCII_WORD, 0, 1) &&
                  atom_eq(a[1], AtomKind::ASCII_WORD, 2, 3),
              "16 stray 0xFF skipped one byte at a time");
    }
    {
        // E5 A4 would start U+5929 but 'a' is not a continuation.
        const auto a = scan_atoms("\xE5\xA4" "a");
        check(a.size() == 1 && atom_eq(a[0], AtomKind::ASCII_WORD, 2, 3),
              "17 truncated sequence cascades byte by byte");
    }
    {
        // F4 90 BF BF would decode above U+10FFFF; the decoder guard
        // and the byte cascade consume the same 4-byte span.
        const auto a = scan_atoms("\xF4\x90\xBF\xBF" "a");
        check(a.size() == 1 && atom_eq(a[0], AtomKind::ASCII_WORD, 4, 5),
              "18 overlong F4 sequence spans 4 bytes then resumes");
    }

    return test_summary();
}
