// tokenizer.cpp
// Implements the contract in include/tokenizer.h.
#include <vector>
#include <string>
#include <cctype>
#include <utility>
#include "tokenizer.h"

// Tokenizes one piece of text into lowercase tokens.
//
// Implementation notes:
// 1. Single scan with two cursors: start marks the first byte of a
//    word, i runs forward while word characters continue; the
//    half-open range [start, i) is one token, cut out with substr.
// 2. A word character is a letter, a digit, or a hyphen; every other
//    character is a separator. The contract's rule, written once in
//    the isWordChar lambda.
// 3. Bytes are cast to unsigned char before isalnum/tolower because
//    passing a negative char value to them is undefined behavior.

std::vector<std::string> tokenize(const std::string& text) {
	std::vector<std::string> tokens;
	std::string::size_type i = 0;
	const auto isWordChar = [](unsigned char ch) {
		return std::isalnum(ch) != 0 || ch == '-';
	};

	while (i < text.size()) {
		if (isWordChar(static_cast<unsigned char>(text[i]))) {
			const std::string::size_type start = i;
			while (i < text.size() &&
				   isWordChar(static_cast<unsigned char>(text[i]))) {
				++i;
			}

			std::string token = text.substr(start, i - start);
			for (char& ch : token) {
				ch = static_cast<char>(
					std::tolower(static_cast<unsigned char>(ch)));
			}
			tokens.emplace_back(std::move(token));
		} 
        else {
			++i;
		}
	}

	return tokens;
}
