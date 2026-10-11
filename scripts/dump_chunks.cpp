// T10 tool (promoted from %TEMP% scratch 2026-10-11): dump corpus
// chunks through the REAL chunker (loader + chunker + atom_scan) as
// JSONL for offline evaluation. Evaluation scripts must never
// re-implement chunking -- the dump keeps the corpus view identical
// to the product's.
//
// Build (repo root, VS x64 command prompt):
//   cl /nologo /utf-8 /std:c++17 /EHsc /W4 /I MY_CTF_RAG\include ^
//      scripts\dump_chunks.cpp MY_CTF_RAG\src\loader.cpp ^
//      MY_CTF_RAG\src\chunker.cpp MY_CTF_RAG\src\atom_scan.cpp ^
//      /Fe:scripts\dump_chunks.exe
// Run (from MY_CTF_RAG, so the "data" relative path resolves):
//   ..\scripts\dump_chunks.exe chunks.jsonl
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "chunker.h"
#include "loader.h"
#include "type.h"

namespace {

std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char raw : s) {
        const unsigned char c = static_cast<unsigned char>(raw);
        switch (raw) {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", c);
                out += buf;
            } else {
                out += raw;
            }
        }
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string output_path = argc > 1 ? argv[1] : "chunks.jsonl";
    const std::vector<Document> docs = load_documents("data");
    const std::vector<Chunk> chunks = chunk_documents(docs);

    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "cannot open output: " << output_path << "\n";
        return 1;
    }
    for (const Chunk& c : chunks) {
        const Document& d =
            docs[static_cast<std::size_t>(c.document_id)];
        out << "{\"id\":" << c.id
            << ",\"doc_id\":" << c.document_id
            << ",\"path\":\"" << json_escape(d.path) << "\""
            << ",\"begin\":" << c.begin
            << ",\"end\":" << c.end
            << ",\"text\":\"" << json_escape(c.text) << "\"}\n";
    }
    std::cout << "dumped " << chunks.size() << " chunks from "
              << docs.size() << " docs -> " << output_path << "\n";
    return 0;
}
