#include "test_framework.h"

#include "game_state.h"

TEST_CASE(no_death_clear_starts_unset_for_a_fresh_state) {
    GameState state{};
    CHECK(!has_no_death_clear(state, 0));
    CHECK(!has_no_death_clear(state, 5));
    CHECK_EQ(state.no_death_clears, 0u);
}

TEST_CASE(no_death_clear_set_bit_is_read_back) {
    GameState state{};
    set_no_death_clear(state, 5);
    CHECK(has_no_death_clear(state, 5));
}

TEST_CASE(no_death_clear_setting_one_bit_does_not_affect_neighbors) {
    GameState state{};
    set_no_death_clear(state, 5);
    CHECK(!has_no_death_clear(state, 4));
    CHECK(!has_no_death_clear(state, 6));
}

TEST_CASE(no_death_clear_tracks_multiple_independent_bits) {
    GameState state{};
    set_no_death_clear(state, 0);
    set_no_death_clear(state, 16);
    set_no_death_clear(state, 23);

    for (unsigned i = 0; i < 32; ++i) {
        const bool expected = (i == 0 || i == 16 || i == 23);
        CHECK_EQ(has_no_death_clear(state, i), expected);
    }
}

TEST_CASE(no_death_clear_out_of_range_index_is_safely_ignored) {
    GameState state{};
    set_no_death_clear(state, 32);  // one past the 32-bit mask's range
    set_no_death_clear(state, 100);

    CHECK_EQ(state.no_death_clears, 0u);
    CHECK(!has_no_death_clear(state, 32));
    CHECK(!has_no_death_clear(state, 100));
}

TEST_CASE(no_death_clear_survives_alongside_other_game_state_fields) {
    // Regression guard: MAX_LEVELS-sized arrays sit right next to the
    // bitmask in the struct - a future field reorder should not corrupt
    // no_death_clears or vice versa.
    GameState state{};
    state.best_time_frames[3] = 12345;
    set_no_death_clear(state, 3);

    CHECK_EQ(state.best_time_frames[3], 12345u);
    CHECK(has_no_death_clear(state, 3));
}