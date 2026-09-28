#pragma once

// tokenizer.h
// Turns raw text into a list of normalized tokens.

#include <string>
#include <vector>

// Tokenizes one piece of text into a list of tokens.
//
// Contract:
// 1. ASCII letters come out lowercase; Han characters have no case.
// 2. Composition policy, in two parts:
//    (a) Each ASCII_WORD atom produces one lowercased token.
//    (b) Within a maximal run of byte-adjacent CJK_CHAR atoms (each
//        atom starts exactly where the previous one ends), every
//        adjacent pair becomes a two-character token (bigram).
//        A run of length 1 emits nothing. Any non-CJK atom or
//        separator terminates the run.
//    The underlying lexical rules (what counts as an ASCII_WORD,
//    what counts as a CJK_CHAR, where their byte spans lie) are
//    defined in atom_scan.h; this function must not re-implement
//    them.
//    Examples (input written as space-separated code points for
//    readability -- a real space in the input appears as U+0020;
//    output tokens concatenate their two code points):
//      ""                              -> {}
//      U+5929 U+4E0B U+65E0 U+654C     -> [U+5929U+4E0B][U+4E0BU+65E0][U+65E0U+654C]
//      U+5929 U+4E0B U+65E0            -> [U+5929U+4E0B][U+4E0BU+65E0]
//      U+5929 U+4E0B                   -> [U+5929U+4E0B]
//      U+5929                          -> {}
//      U+5929 U+0020 U+4E0B            -> {}
//      ROP U+94FE                      -> [rop]
//      U+5929 U+4E0B ROP U+65E0 U+654C -> [U+5929U+4E0B][rop][U+65E0U+654C]
//      RET2LIBC                        -> [ret2libc]
//    A malformed byte between two Han characters breaks the run:
//    the scanner skips it, so the two atoms are not byte-adjacent.
// 3. Indexing and querying must use this same function; mismatched
//    rules on either end break term matching.
// 4. Tokens come out in order of appearance; empty input yields
//    an empty vector.
std::vector<std::string> tokenize(const std::string& text);
