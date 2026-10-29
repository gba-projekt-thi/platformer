#include "test_framework.h"

#include "level_records.h"

TEST_CASE(records_first_clear_is_a_new_best) {
    GameState state;
    CHECK(record_clear(state, 3, 1200, false, true));
    CHECK_EQ(state.best_time_frames[3], 1200u);
}

TEST_CASE(records_faster_clear_replaces_the_best) {
    GameState state;
    record_clear(state, 3, 1200, false, true);
    CHECK(record_clear(state, 3, 1100, false, true));
    CHECK_EQ(state.best_time_frames[3], 1100u);
}

TEST_CASE(records_slower_or_equal_clear_keeps_the_best) {
    GameState state;
    record_clear(state, 3, 1200, false, true);
    CHECK(!record_clear(state, 3, 1300, false, true));
    CHECK(!record_clear(state, 3, 1200, false, true));
    CHECK_EQ(state.best_time_frames[3], 1200u);
}

TEST_CASE(records_no_death_clear_is_marked_even_without_a_new_best) {
    GameState state;
    record_clear(state, 5, 900, false, true);
    CHECK(!has_no_death_clear(state, 5));
    CHECK(!record_clear(state, 5, 1000, true, true));
    CHECK(has_no_death_clear(state, 5));
}

TEST_CASE(records_a_clear_with_deaths_never_removes_the_no_death_mark) {
    GameState state;
    record_clear(state, 5, 900, true, true);
    record_clear(state, 5, 800, false, true);
    CHECK(has_no_death_clear(state, 5));
}

TEST_CASE(records_nothing_is_recorded_while_records_are_not_allowed) {
    // Assist Mode (LevelManager::records_allowed()).
    GameState state;
    CHECK(!record_clear(state, 2, 700, true, false));
    CHECK_EQ(state.best_time_frames[2], 0u);
    CHECK(!has_no_death_clear(state, 2));
}

TEST_CASE(records_levels_beyond_the_save_are_ignored) {
    GameState state;
    const GameState before = state;
    CHECK(
        !record_clear(state, unsigned(GameState::MAX_LEVELS), 700, true, true));
    for (int i = 0; i < GameState::MAX_LEVELS; ++i) {
        CHECK_EQ(state.best_time_frames[i], before.best_time_frames[i]);
    }
}

TEST_CASE(records_other_levels_are_untouched) {
    GameState state;
    record_clear(state, 4, 500, true, true);
    CHECK_EQ(state.best_time_frames[3], 0u);
    CHECK_EQ(state.best_time_frames[5], 0u);
    CHECK(!has_no_death_clear(state, 3));
}

TEST_CASE(records_mirror_clear_has_its_own_best_time_and_badge) {
    GameState state{};
    CHECK(record_clear(state, 3, 900, true, true));
    CHECK(record_clear(state, 3, 1500, false, true, true));
    CHECK_EQ(state.best_time_frames[3], 900u);
    CHECK_EQ(state.best_time_frames_mirror[3], 1500u);
    CHECK(has_no_death_clear(state, 3));
    CHECK(!has_no_death_clear(state, 3, true));
    CHECK(!record_clear(state, 3, 1600, true, true, true));
    CHECK_EQ(state.best_time_frames_mirror[3], 1500u);
    CHECK(has_no_death_clear(state, 3, true));
    CHECK_EQ(best_time(state, 3, true), 1500u);
    CHECK_EQ(best_time(state, 3), 900u);
    CHECK_EQ(best_time(state, 99, true), 0u);
}
