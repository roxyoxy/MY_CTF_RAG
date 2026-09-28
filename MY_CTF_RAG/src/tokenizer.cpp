#include "tokenizer.h"

#include <utility>

#include "atom_scan.h"

namespace {

    // ASCII-only lowercase. std::tolower is locale-dependent, so we
    // roll our own range-based version to keep MSVC and g++ in sync.
    char ascii_to_lower(char c) {
        unsigned char u = static_cast<unsigned char>(c);
        if (u >= 'A' && u <= 'Z') {
            return static_cast<char>(u - 'A' + 'a');
        }
        return c;
    }

}  // namespace

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    const std::vector<Atom> atoms = scan_atoms(text);
    const size_t n = atoms.size();

    size_t i = 0;
    while (i < n) {
        const Atom& atom = atoms[i];

        if (atom.kind == AtomKind::ASCII_WORD) {
            // Clause 2(a): one lowercased token per ASCII atom.
            std::string tok(text, atom.begin, atom.end - atom.begin);
            for (char& c : tok) {
                c = ascii_to_lower(c);
            }
            tokens.push_back(std::move(tok));
            ++i;
            continue;
        }

        // Clause 2(b): collect a maximal run of byte-adjacent
        // CJK_CHAR atoms. A separator leaves a byte gap, so the
        // next atom's begin does not equal the previous atom's end,
        // and the run stops there.
        size_t run_end = i + 1;
        while (run_end < n &&
            atoms[run_end].kind == AtomKind::CJK_CHAR &&
            atoms[run_end - 1].end == atoms[run_end].begin) {
            ++run_end;
        }

        // n-1 bigrams per run; a run of length 1 emits nothing
        // (isolated Han character).
        for (size_t k = i; k + 1 < run_end; ++k) {
            std::string bigram(text, atoms[k].begin,
                atoms[k].end - atoms[k].begin);
            bigram.append(text, atoms[k + 1].begin,
                atoms[k + 1].end - atoms[k + 1].begin);
            tokens.push_back(std::move(bigram));
        }

        i = run_end;
    }

    return tokens;
}