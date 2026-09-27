// check.h
// Tiny PASS/FAIL counter shared by all test binaries.
// Usage: check(cond, "case name"); ... at the end: return test_summary();
// Exit code 0 = all pass, 1 = any failure (so scripts can gate on it).
#pragma once
#include <iostream>
#include <string>

inline int g_passed = 0;
inline int g_failures = 0;

inline void check(bool ok, const std::string& name) {
    if (ok) { ++g_passed; std::cout << "PASS " << name << "\n"; }
    else    { ++g_failures; std::cout << "FAIL " << name << "\n"; }
}

inline int test_summary() {
    std::cout << (g_failures == 0 ? "ALL PASS" : "HAS FAILURES")
              << " passed=" << g_passed << " failures=" << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
