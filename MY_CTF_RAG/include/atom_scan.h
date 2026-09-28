#pragma once

// atom_scan.h
// Scans UTF-8 text into atoms: the single source of truth for
// lexical units in this project.

#include <cstddef>
#include <string>
#include <vector>

enum class AtomKind {
    ASCII_WORD,// Maximal run of ASCII word characters.
    CJK_CHAR,// One Han code point, 3 UTF-8 bytes.
};

struct Atom {
    size_t begin;// Byte offset into the input text, half-open [begin, end).
    size_t end;// One past the last byte of the unit.
    AtomKind kind;
};
// Atom: an unsplittable lexical unit with a span pointing back into
// the source text. Unlike Chunk::text, no copy of the bytes is stored;
// atoms are short-lived intermediates consumed by tokenize() and
// chunk_documents() within the same call chain.

// Scans text into an ordered list of atoms.
//
// Contract:
// 1. Atoms come out in order of appearance and never overlap.
//    Separator bytes belong to no atom. Empty input or separator-only
//    input yields an empty vector.
// 2. ASCII_WORD:
//    - a maximal run of [A-Za-z0-9]
//    - '-' is unconditionally word-internal
//    - '.' and '_' are word-internal only when the characters on both
//      sides are [A-Za-z0-9] (the sandwich rule)
//    Examples: libc-2.31, 127.0.0.1, exploit.py and buf_size stay
//    whole; done., wait..., __libc_csu_init and e.g. split -- in
//    e.g. the first '.' joins the word, the trailing one does not.
//    No lowercasing happens here: an atom is a byte span, not a word;
//    normalization belongs to tokenize().
// 3. CJK_CHAR: one code point in U+3400..U+4DBF or U+4E00..U+9FFF,
//    the span covering its full 3-byte UTF-8 sequence. For example
//    U+5929 U+4E0B ("tian xia") yields two CJK_CHAR atoms.
// 4. begin/end are byte offsets into the input text, half-open
//    [begin, end); same type and meaning as Chunk::begin/end, so chunk
//    cut points and future GUI highlights can point back at the text.
// 5. Everything else is a separator and produces no atom: whitespace,
//    punctuation (half- and full-width), emoji, and other scripts.
//    U+FF0C (full-width comma) and U+FF21 (full-width A) are not
//    atoms; anything that is not an ASCII_WORD or a CJK_CHAR is a
//    separator.
// 6. Malformed UTF-8 (illegal or truncated sequences, surrogates,
//    code points above U+10FFFF) is survived: skip one byte and keep
//    scanning. This function never throws and always returns for any
//    byte input, so a dirty writeup can degrade retrieval but can
//    never crash or paralyze index building.
// 7. Single source of truth: tokenizer and chunker must obtain their
//    lexical units through this function; no module may implement
//    its own character-classification rules.
std::vector<Atom> scan_atoms(const std::string& text);
