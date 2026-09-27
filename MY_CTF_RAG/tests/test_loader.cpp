// test_loader.cpp -- contract tests for loader.h (T2).
// Builds a throwaway directory tree in CWD, exercises the loader,
// then cleans up after itself.
#include "check.h"
#include "loader.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << content;
}

int main() {
    const fs::path root = "test_loader_tmp";

    // Missing directory must throw.
    bool threw = false;
    try { (void)load_documents("no_such_dir_for_loader_test"); }
    catch (const std::exception&) { threw = true; }
    check(threw, "1 nonexistent dir throws");

    // Empty dir: empty vector, no throw.
    fs::remove_all(root);
    fs::create_directories(root / "pwn");
    check(load_documents(root.string()).empty(),
          "2 empty dir -> empty vector");

    // Populate: out-of-order names, mixed-case extension, nested
    // subdirs, one ignored extension.
    write_file(root / "web" / "b_second.MD", "alpha content");
    write_file(root / "web" / "a_first.txt", "beta content with CRLF\r\nline2");
    write_file(root / "pwn" / "c_third.md", "gamma");
    write_file(root / "pwn" / "ignored.exe", "should not appear");

    const std::vector<Document> docs = load_documents(root.string());

    check(docs.size() == 3,
          "3 collects .md/.txt case-insensitively, skips other extensions");
    check(docs.size() == 3 &&
              docs[0].path == "pwn/c_third.md" &&
              docs[1].path == "web/a_first.txt" &&
              docs[2].path == "web/b_second.MD",
          "4 sorted relative paths with forward slashes");
    check(docs.size() == 3 &&
              docs[0].id == 0 && docs[1].id == 1 && docs[2].id == 2,
          "5 ids assigned in sorted order");
    check(docs.size() == 3 &&
              docs[1].content == "beta content with CRLF\r\nline2",
          "6 content byte-identical, CRLF preserved");
    check(docs.size() == 3 && !docs[0].deleted,
          "7 deleted flag defaults to false");

    fs::remove_all(root);
    return test_summary();
}
