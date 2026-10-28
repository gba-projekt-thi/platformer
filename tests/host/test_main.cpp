#include "test_framework.h"

// Usage: host_tests [filter] - runs every test whose name contains `filter`.
auto main(int argc, char** argv) -> int {
    const char* filter = argc > 1 ? argv[1] : nullptr;
    const auto& tests = testfw::registry();
    int ran = 0;

    for (const testfw::TestCase& test : tests) {
        if (!testfw::selected(test.name, filter)) {
            continue;
        }
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
    // A filter that matches nothing is a typo, not a pass.
    if (ran == 0) {
        std::printf(
            "No test matches \"%s\".\n", filter != nullptr ? filter : "");
        return 1;
    }
    return testfw::g_failures == 0 ? 0 : 1;
}
