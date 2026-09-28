// test_chunker.cpp -- contract tests for chunker.h (T3).
// Includes the sandwich-isomorphism check against tokenize().
#include "check.h"
#include "chunker.h"
#include "tokenizer.h"
#include "type.h"
#include <string>
#include <vector>

static Document makeDoc(int id, const std::string& content) {
    Document d;
    d.id = id;
    d.path = "d.md";
    d.content = content;
    d.deleted = false;
    return d;
}

int main() {
    // The chunker's own word scan must agree with tokenize() on the
    // tricky sandwich cases (same rule, two implementations).
    const std::string tricky =
        "done. wait... 127.0.0.1 exploit.py buf_size __libc_csu_init "
        "libc-2.31 _emphasis_ e.g. format-string use-after-free";
    const auto toks = tokenize(tricky);
    const auto chunks1 = chunk_documents({makeDoc(0, tricky)});
    check(chunks1.size() == 1 && tokenize(chunks1[0].text) == toks,
          "1 chunker word scan matches tokenize() (sandwich isomorphic)");

    // begin/end are whole-word byte offsets; gaps are cut.
    const std::string lead = "   hello sandwich.world tail   ";
    const auto chunks2 = chunk_documents({makeDoc(0, lead)});
    check(chunks2.size() == 1 && chunks2[0].begin == 3 &&
              chunks2[0].end == static_cast<int>(lead.rfind("tail")) + 4 &&
              lead.substr(chunks2[0].begin, chunks2[0].end - chunks2[0].begin) ==
                  chunks2[0].text,
          "2 begin/end whole-word byte offsets, text = substr");

    // 600 words -> 2 chunks; overlap 50 words; second starts at w450.
    std::string big;
    for (int i = 0; i < 600; ++i) big += "w" + std::to_string(i) + " ";
    const auto chunks3 = chunk_documents({makeDoc(0, big)});
    check(chunks3.size() == 2,
          "3 600 words -> 2 chunks");
    check(chunks3.size() == 2 &&
              tokenize(chunks3[0].text).size() == CHUNK_SIZE &&
              tokenize(chunks3[1].text).front() ==
                  "w" + std::to_string(CHUNK_SIZE - CHUNK_OVERLAP) &&
              tokenize(chunks3[1].text).back() == "w599" &&
              tokenize(chunks3[1].text).size() == 150,
          "4 overlap = 50 words, second chunk starts at w450");

    // Empty / separator-only docs produce no chunks.
    check(chunk_documents({makeDoc(0, "")}).empty() &&
              chunk_documents({makeDoc(0, "  ... !!! ??? ,,, ")}).empty(),
          "5 empty and separator-only docs -> 0 chunks");

    // Hyphens are unconditional word chars: a bare --- is one word.
    const auto hyph = chunk_documents({makeDoc(0, "  ... --- !!! ")});
    check(hyph.size() == 1 && hyph[0].text == "---",
          "6 standalone hyphens form one word");

    // Chunk ids are global and contiguous across documents.
    const auto chunks7 = chunk_documents(
        {makeDoc(0, "a b c"), makeDoc(1, "d e"), makeDoc(2, "f")});
    check(chunks7.size() == 3 && chunks7[0].id == 0 && chunks7[1].id == 1 &&
              chunks7[2].id == 2 && chunks7[1].document_id == 1,
          "7 global contiguous chunk ids, document_id preserved");

    // M2 Chinese layer: atoms are the counting unit (1 Han char = 1
    // atom), byte offsets come from the atom table.
    const auto han = [](int n) {
        std::string s;
        for (int i = 0; i < n; ++i) s += "\u5929";
        return s;
    };

    // 501 Han chars -> 2 chunks; counting bigrams instead of atoms
    // would yield 1 chunk (501 chars = 500 bigrams).
    const auto c8 = chunk_documents({makeDoc(0, han(501))});
    check(c8.size() == 2 && c8[0].begin == 0 && c8[0].end == 1500 &&
              c8[1].begin == 1350 && c8[1].end == 1503,
          "8 501 Han chars -> 2 chunks (atoms, not bigrams)");

    // Full-width punctuation yields no atom: 499 + 1 = 500 atoms.
    const auto c9 =
        chunk_documents({makeDoc(0, han(499) + "\u3002" + han(1))});
    check(c9.size() == 1 && c9[0].begin == 0 && c9[0].end == 1503,
          "9 full-width stop yields no atom (500 atoms -> 1 chunk)");

    // Malformed bytes yield no atom and swallow no neighbors.
    const auto c10 = chunk_documents({makeDoc(0, "\u5929" "\xFF\xFF" "\u4E0B")});
    check(c10.size() == 1 && c10[0].begin == 0 && c10[0].end == 8,
          "10 malformed bytes skipped, neighbors intact");

    // 600 Han chars: overlap of 50 atoms = 150 byte-identical bytes.
    const auto c11 = chunk_documents({makeDoc(0, han(600))});
    check(c11.size() == 2 && c11[1].begin == 1350 && c11[1].end == 1800 &&
              c11[0].text.substr(1350, 150) == c11[1].text.substr(0, 150),
          "11 overlap of 50 atoms is 150 identical bytes");

    // ASCII words and Han chars weigh the same: 450 + 1 + 49 = 500.
    const auto c12 =
        chunk_documents({makeDoc(0, han(450) + " libc " + han(49))});
    check(c12.size() == 1,
          "12 ASCII and CJK atoms weigh the same");

    return test_summary();
}
