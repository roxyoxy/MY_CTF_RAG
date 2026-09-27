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

    return test_summary();
}
