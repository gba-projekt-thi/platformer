#include "test_framework.h"

#include "attempt_variant.h"

using attempt_variant::active_for_deaths;
using attempt_variant::deaths_in_level;
using attempt_variant::variant_for_attempt;

TEST_CASE(attempt_deaths_in_level_count_from_the_level_entry) {
    CHECK(deaths_in_level(0, 0) == 0);
    CHECK(deaths_in_level(7, 4) == 3);
    // The run's total never drops below the entry value; if it does (a save
    // was loaded), the level starts fresh instead of wrapping around.
    CHECK(deaths_in_level(2, 5) == 0);
}

TEST_CASE(attempt_window_zero_zero_is_always_there) {
    // The default of every trap and platform so far.
    for (unsigned deaths : {0u, 1u, 3u, 100u}) {
        CHECK(active_for_deaths(deaths, 0, 0));
    }
}

TEST_CASE(attempt_window_from_shows_up_after_that_many_deaths) {
    CHECK(!active_for_deaths(0, 3, 0));
    CHECK(!active_for_deaths(2, 3, 0));
    CHECK(active_for_deaths(3, 3, 0));
    CHECK(active_for_deaths(50, 3, 0));
}

TEST_CASE(attempt_window_until_is_gone_from_that_death_on) {
    CHECK(active_for_deaths(0, 0, 3));
    CHECK(active_for_deaths(2, 0, 3));
    CHECK(!active_for_deaths(3, 0, 3));
}

TEST_CASE(attempt_two_windows_split_at_the_same_death_never_overlap) {
    // "After the 3rd death the trap sits somewhere else": exactly one of the
    // two copies is there on every attempt.
    for (unsigned deaths = 0; deaths < 10; ++deaths) {
        CHECK(
            active_for_deaths(deaths, 0, 3) != active_for_deaths(deaths, 3, 0));
    }
}

TEST_CASE(attempt_empty_window_is_never_there) {
    // until <= from (and until != 0): authored by mistake, shows nothing.
    for (unsigned deaths = 0; deaths < 10; ++deaths) {
        CHECK(!active_for_deaths(deaths, 4, 4));
        CHECK(!active_for_deaths(deaths, 5, 2));
    }
}

TEST_CASE(attempt_variant_switches_every_n_deaths_and_stays_at_the_last) {
    // 3 variants, a new one every 2 deaths: 0 0 1 1 2 2 2 2 ...
    const unsigned expected[] = {0, 0, 1, 1, 2, 2, 2, 2};
    for (unsigned deaths = 0; deaths < 8; ++deaths) {
        CHECK(variant_for_attempt(deaths, 2, 3) == expected[deaths]);
    }
}

TEST_CASE(attempt_variant_degenerate_inputs_pick_the_first) {
    CHECK(variant_for_attempt(5, 0, 3) == 0);  // never switches
    CHECK(variant_for_attempt(5, 2, 0) == 0);  // no variants
    CHECK(variant_for_attempt(5, 2, 1) == 0);  // only one
}
