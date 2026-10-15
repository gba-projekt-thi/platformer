#pragma once

// -----------------------------------------------------------------------------
// test_framework.h
//
// A minimal, header-only test harness for the host-side unit tests under
// tests/host/. Deliberately not a real framework (no GoogleTest/Catch2
// dependency to install or vendor) - just enough to register test cases,
// run them, and report pass/fail with a non-zero process exit code on
// failure, which is all a CI step needs.
//
// Usage:
//   #include "test_framework.h"
//
//   TEST_CASE(my_test_name) {
//       CHECK(1 + 1 == 2);
//       CHECK_EQ(compute(), 42);
//   }
//
// main() (see test_main.cpp) iterates every registered case and runs it.
// -----------------------------------------------------------------------------

#include <cstdio>
#include <vector>

namespace testfw {

struct TestCase {
    const char* name;
    void (*fn)();
};

// Function-local static avoids the static-initialization-order fiasco that
// a plain global std::vector of registrars would risk.
inline auto registry() -> std::vector<TestCase>& {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) {
        registry().push_back({name, fn});
    }
};

inline int g_failures = 0;
inline const char* g_current_test = "";

}  // namespace testfw

#define TEST_CASE(name)                                       \
    static void name();                                       \
    static ::testfw::Registrar registrar_##name(#name, name); \
    static void name()

#define CHECK(expr)                                                   \
    do {                                                              \
        if (!(expr)) {                                                \
            std::fprintf(                                             \
                stderr, "  FAIL [%s] %s:%d: CHECK(%s)\n",             \
                ::testfw::g_current_test, __FILE__, __LINE__, #expr); \
            ++::testfw::g_failures;                                   \
        }                                                             \
    } while (0)

#define CHECK_EQ(actual, expected)                                     \
    do {                                                               \
        auto actual_value = (actual);                                  \
        auto expected_value = (expected);                              \
        if (!(actual_value == expected_value)) {                       \
            std::fprintf(                                              \
                stderr, "  FAIL [%s] %s:%d: CHECK_EQ(%s, %s)\n",       \
                ::testfw::g_current_test, __FILE__, __LINE__, #actual, \
                #expected);                                            \
            ++::testfw::g_failures;                                    \
        }                                                              \
    } while (0)
