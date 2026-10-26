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
// main() (see test_main.cpp) iterates every registered case and runs it;
// `./host_tests crumble` runs only the cases whose name contains "crumble".
// A failing CHECK_EQ prints both values (see to_text()).
// -----------------------------------------------------------------------------

#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
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

// Readable form of a checked value: numbers (enums by their value), bools as
// words, chars and strings quoted; anything else as "<?>".
template <typename T>
auto to_text(const T& value) -> std::string {
    using U = std::decay_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_same_v<U, char>) {
        return std::string("'") + value + "'";
    } else if constexpr (
        std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
        if constexpr (std::is_array_v<T>) {
            return "\"" + std::string(value) + "\"";
        } else {
            return value != nullptr ? "\"" + std::string(value) + "\""
                                    : "nullptr";
        }
    } else if constexpr (std::is_same_v<U, std::string>) {
        return "\"" + value + "\"";
    } else if constexpr (std::is_enum_v<U>) {
        return std::to_string(+static_cast<std::underlying_type_t<U>>(value));
    } else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>) {
        return std::to_string(static_cast<long long>(value));
    } else if constexpr (std::is_integral_v<U>) {
        return std::to_string(static_cast<unsigned long long>(value));
    } else if constexpr (std::is_floating_point_v<U>) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%g", static_cast<double>(value));
        return buffer;
    } else {
        return "<?>";
    }
}

// Whether the test `name` runs for the command-line `filter` (a substring of
// the name; null or empty runs every test).
inline auto selected(const char* name, const char* filter) -> bool {
    return filter == nullptr || *filter == '\0' ||
           std::strstr(name, filter) != nullptr;
}

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
                stderr,                                                \
                "  FAIL [%s] %s:%d: CHECK_EQ(%s, %s)\n"                \
                "         actual: %s\n       expected: %s\n",          \
                ::testfw::g_current_test, __FILE__, __LINE__, #actual, \
                #expected, ::testfw::to_text(actual_value).c_str(),    \
                ::testfw::to_text(expected_value).c_str());            \
            ++::testfw::g_failures;                                    \
        }                                                              \
    } while (0)
