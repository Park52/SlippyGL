#pragma once
// Tiny dependency-free assertion helper for SlippyGL unit tests.
// Uses C++17 inline variables so the counters have a single definition
// across all test translation units.
#include <cstdio>
#include <cmath>

namespace slippytest
{
    inline int g_checks = 0;
    inline int g_fails  = 0;

    inline void report(bool ok, const char* expr, const char* file, int line)
    {
        ++g_checks;
        if (!ok) {
            ++g_fails;
            std::printf("  FAIL: %s   (%s:%d)\n", expr, file, line);
        }
    }
}

#define CHECK(cond)        ::slippytest::report((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b)     ::slippytest::report((a) == (b), #a " == " #b, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, eps) \
    ::slippytest::report(std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= (eps), \
                         #a " ~= " #b, __FILE__, __LINE__)
