#include "test_framework.h"

#include "level_limits.h"

TEST_CASE(level_limits_a_level_exactly_at_the_limit_fits) {
    CHECK(level_limits::within(32, 32));
    CHECK(level_limits::within(16, 16));
    CHECK(level_limits::within(4, 4));
}

TEST_CASE(level_limits_one_over_the_limit_does_not_fit) {
    CHECK(!level_limits::within(33, 32));
    CHECK(!level_limits::within(5, 4));
}

TEST_CASE(level_limits_below_the_limit_and_empty_fit) {
    CHECK(level_limits::within(0, 32));
    CHECK(level_limits::within(31, 32));
    CHECK(level_limits::within(0, 0));
}

TEST_CASE(level_limits_a_zero_limit_only_fits_nothing) {
    CHECK(!level_limits::within(1, 0));
}

TEST_CASE(level_limits_negative_counts_never_fit) {
    // A negative count converted to unsigned would be huge and fail anyway,
    // but must not depend on that - and must not fit a large limit either.
    CHECK(!level_limits::within(-1, 32));
    CHECK(!level_limits::within(-1, 0xFFFFFFFFu));
}

TEST_CASE(level_limits_large_limits_do_not_overflow) {
    CHECK(level_limits::within(2147483647, 2147483647u));
    CHECK(!level_limits::within(2147483647, 100u));
}
