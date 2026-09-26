// loader.cpp
// Implements the contract in include/loader.h.
#include "loader.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Content is the full file text, UTF-8.
// Recursively loads all .md and .txt files under dir.
// Paths are stored relative to dir using forward slashes.
// Files are sorted lexicographically before numbering, ids start from 0.
// Throws an exception if the directory does not exist.
std::vector<Document> load_documents(const std::string& dir){
    namespace fs = std::filesystem;

    // Normalize the input path and fail immediately if it is not a directory.
    const fs::path root = fs::path(dir).lexically_normal();
    std::error_code error;
    if (!fs::is_directory(root, error)) {
        if (error) {
            throw std::runtime_error(
                "Unable to access directory '" + dir + "': " + error.message());
        }
        throw std::runtime_error("Directory does not exist or is not a directory: " + dir);
    }

    // Recursively collect regular Markdown and text files.
    std::vector<Document> documents;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        // Normalize the extension to lowercase so matching is case-insensitive.
        std::string extension = entry.path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
        if (extension != ".md" && extension != ".txt") {
            continue;
        }

        // Store each file's root-relative path using generic forward-slash separators.
        const fs::path relative_path =
            entry.path().lexically_relative(root).lexically_normal();
        documents.push_back({0, relative_path.generic_string(), "", false});
    }

    // Sort by relative path before assigning stable, sequential document IDs.
    std::sort(documents.begin(), documents.end(),
        [](const Document& left, const Document& right) {
            return left.path < right.path;
        });

    // Read each file as raw bytes; binary mode prevents text-mode transformations.
    for (std::size_t i = 0; i < documents.size(); ++i) {
        Document& document = documents[i];
        document.id = static_cast<int>(i);

        const fs::path full_path = root / fs::path(document.path);
        std::ifstream input(full_path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Unable to open file: " + full_path.string());
        }

        std::ostringstream content;
        content << input.rdbuf();
        if (input.bad() || content.bad()) {
            throw std::runtime_error("Unable to read file: " + full_path.string());
        }
        document.content = content.str();
    }

    // An empty or file-free directory naturally returns an empty vector.
    return documents;
}
