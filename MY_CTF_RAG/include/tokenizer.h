#pragma once

// tokenizer.h
// Turns raw text into a list of normalized tokens.

#include <string>
#include <vector>

// Tokenizes one piece of text into a list of tokens.
//
// Contract:
// 1. Lowercases everything.
// 2. A token is a maximal run of letters, digits, and hyphens;
//    every other character is a separator.
// 3. Indexing and querying must use this same function; mismatched
//    rules on either end break term matching.
// 4. Tokens come out in order of appearance; empty input yields
//    an empty vector.
std::vector<std::string> tokenize(const std::string& text);