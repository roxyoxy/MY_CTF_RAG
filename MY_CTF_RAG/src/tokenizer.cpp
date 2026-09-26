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
// 2. Word characters are letters, digits, and hyphens; '.' and '_'
//    count only when both neighbors are letters or digits (the
//    sandwich rule, tokenizer.h clause 2). The predicate is
//    position-aware: isWordCharAt(position) captures text and checks
//    both neighbors, so it is not a plain char predicate.
// 3. Bytes are cast to unsigned char before isalnum/tolower because
//    passing a negative char value to them is undefined behavior.

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string::size_type i = 0;
    const auto isAlphaNum = [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0;
    };
    const auto isWordCharAt = [&text, &isAlphaNum](std::string::size_type position) {
        const char ch = text[position];
        if (isAlphaNum(ch) || ch == '-') {
            return true;
        }

        return (ch == '.' || ch == '_') &&
               position > 0 &&
               position + 1 < text.size() &&
               isAlphaNum(text[position - 1]) &&
               isAlphaNum(text[position + 1]);
    };

    while (i < text.size()) {
        if (isWordCharAt(i)) {
            const std::string::size_type start = i;
            while (i < text.size() && isWordCharAt(i)) {
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

// Self-test cases (all must pass before closing the task;
// decision record: MEETING_LOG 2026-09-26 AI-C):
// - "Hello, World"           -> [hello] [world]
// - "use-after-free"         -> [use-after-free]        hyphen kept
// - ""                       -> (empty vector)
// - "ret2libc NX libc-2.31"  -> [ret2libc] [nx] [libc-2.31]   version dot kept
// - "Phrack #49!"            -> [phrack] [49]           digits are word chars
// - "It costs 2.31."         -> [it] [costs] [2.31]     sentence period splits
// - "wait..."                -> [wait]                  ellipsis splits
// - "127.0.0.1"              -> [127.0.0.1]             IP kept whole
// - "exploit.py"             -> [exploit.py]            filename kept whole
// - "buf_size"               -> [buf_size]              inner underscore kept
// - "__libc_csu_init"        -> [libc_csu_init]         leading underscores split
// - "_emphasis_"             -> [emphasis]              markdown underscores split
// - "e.g."                   -> [e.g]                   known cosmetic edge
