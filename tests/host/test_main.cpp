#include "test_framework.h"

auto main() -> int {
    const auto& tests = testfw::registry();
    int ran = 0;

    for (const testfw::TestCase& test : tests) {
        testfw::g_current_test = test.name;
        const int failures_before = testfw::g_failures;
        test.fn();
        ++ran;
        if (testfw::g_failures == failures_before) {
            std::printf("  ok   %s\n", test.name);
        }
    }

    std::printf(
        "\n%d test case(s) ran, %d failure(s).\n", ran, testfw::g_failures);
    return testfw::g_failures == 0 ? 0 : 1;
}