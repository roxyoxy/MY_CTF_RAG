#include "atom_scan.h"

#include <cstdint>
#include <utility>

namespace {

    // Returns true if c is an ASCII digit, lowercase, or uppercase letter.
    // Uses explicit ranges instead of std::isalnum to stay locale-independent:
    // MSVC and g++ must agree byte-for-byte.
    bool is_alnum(unsigned char c) {
        return (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z');
    }

    // Returns true if c is an ASCII word character: alnum or hyphen.
    // The hyphen is unconditionally word-internal, including at the
    // leading and trailing edges of a word.
    bool is_word_char(unsigned char c) {
        return is_alnum(c) || c == '-';
    }

    // Scans an ASCII word starting at text[start] and returns the end
    // position (exclusive).
    //
    // Precondition: text[start] is a word character, i.e. in [A-Za-z0-9-].
    // The caller (scan_atoms) guarantees this, so text[j - 1] is always
    // safe to read inside the loop.
    //
    // Word rules:
    //   - [A-Za-z0-9-] always stays inside the word
    //   - '.' and '_' stay inside only when both neighbors are alnum
    //     (the sandwich rule)
    //
    // Examples:
    //   libc-2.31        -> whole word
    //   127.0.0.1        -> whole word
    //   exploit.py       -> whole word
    //   buf_size         -> whole word
    //   done.            -> "done"    (trailing '.' has no right neighbor)
    //   wait...          -> "wait"    (2nd '.' has a '.' on its left)
    //   __libc_csu_init  -> "libc_csu_init"  (leading '_' has no left neighbor)
    //   a.-b             -> "a" then "-b"    (right neighbor of '.' is '-')
    size_t scan_ascii_word(const std::string& text, size_t start) {
        size_t j = start;
        const size_t n = text.size();

        while (j < n) {
            unsigned char c = static_cast<unsigned char>(text[j]);

            if (is_word_char(c)) {
                ++j;
            }
            else if (c == '.' || c == '_') {
                // Sandwich rule: both neighbors must be alnum.
                // Note that '-' is NOT alnum, so "a.-b" stops here.
                // j >= 1 always holds here: the word start set excludes
                // '.' and '_', so text[j - 1] never underflows.
                if (!is_alnum(static_cast<unsigned char>(text[j - 1]))) break;
                if (j + 1 >= n) break;
                if (!is_alnum(static_cast<unsigned char>(text[j + 1]))) break;
                ++j;
            }
            else {
                break;
            }
        }

        return j;
    }

    // Decodes one UTF-8 sequence starting at text[i].
    //
    // Returns (length_in_bytes, code_point) on success,
    // or (0, 0) on any malformed input:
    //   - stray continuation byte (0x80..0xBF)
    //   - overlong 2-byte lead (0xC0, 0xC1)
    //   - lead beyond U+10FFFF (0xF5..0xFF)
    //   - truncated tail (i + len > text.size())
    //   - a byte in the middle that is not a continuation byte
    //   - decoded code point above U+10FFFF
    //
    // The last check catches 4-byte sequences whose lead byte is
    // F4 but whose continuation bytes push the code point past
    // U+10FFFF (e.g. F4 90 80 80 = U+110000). It is defense-in-depth:
    // through scan_atoms the two paths consume the same bytes anyway
    // (the skip-one-byte cascade eats the continuation bytes one by
    // one), so the check cannot be observed from outside. It is kept
    // so the decoder keeps its own promise: only valid code points.
    std::pair<int, uint32_t> try_decode_utf8(const std::string& text, size_t i) {
        unsigned char b = static_cast<unsigned char>(text[i]);

        int len = 0;
        uint32_t cp = 0;

        if (b >= 0xC2 && b <= 0xDF) {
            len = 2;
            cp = b & 0x1F;
        }
        else if (b >= 0xE0 && b <= 0xEF) {
            len = 3;
            cp = b & 0x0F;
        }
        else if (b >= 0xF0 && b <= 0xF4) {
            len = 4;
            cp = b & 0x07;
        }
        else {
            return { 0, 0 };
        }

        for (int k = 1; k < len; ++k) {
            if (i + static_cast<size_t>(k) >= text.size()) {
                return { 0, 0 };
            }
            unsigned char cont = static_cast<unsigned char>(text[i + k]);
            if (cont < 0x80 || cont > 0xBF) {
                return { 0, 0 };
            }
            cp = (cp << 6) | (cont & 0x3F);
        }

        if (cp > 0x10FFFF) {
            return { 0, 0 };
        }

        return { len, cp };
    }

}  // namespace

std::vector<Atom> scan_atoms(const std::string& text) {
    std::vector<Atom> atoms;
    size_t i = 0;
    const size_t n = text.size();

    while (i < n) {
        unsigned char b = static_cast<unsigned char>(text[i]);

        if (b < 0x80) {
            // ASCII region.
            if (is_word_char(b)) {
                size_t j = scan_ascii_word(text, i);
                atoms.push_back({ i, j, AtomKind::ASCII_WORD });
                i = j;
            }
            else {
                ++i;  // ASCII separator, skip one byte.
            }
        }
        else {
            // Multi-byte region.
            auto [len, cp] = try_decode_utf8(text, i);
            if (len == 0) {
                ++i;  // Clause 6: malformed byte, skip one byte.
            }
            else {
                if ((cp >= 0x3400u && cp <= 0x4DBFu) ||
                    (cp >= 0x4E00u && cp <= 0x9FFFu)) {
                    atoms.push_back(
                        { i, i + static_cast<size_t>(len), AtomKind::CJK_CHAR });
                }
                i += static_cast<size_t>(len);
            }
        }
    }

    return atoms;
}