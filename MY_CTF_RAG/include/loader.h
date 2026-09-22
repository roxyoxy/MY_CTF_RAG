#pragma once
// loader.h
// Loads documents from disk.
#include <string>
#include <vector>
#include "type.h"

// Content is the full file text, UTF-8.
// Recursively loads all .md and .txt files under dir.
// Paths are stored relative to dir using forward slashes.
// Files are sorted lexicographically before numbering, ids start from 0.
// Throws an exception if the directory does not exist.
std::vector<Document> load_documents(const std::string& dir);