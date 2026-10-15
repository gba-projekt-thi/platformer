#include "test_framework.h"

#include "game_modes.h"

using modes::TrapSpeedMode;

TEST_CASE(modes_default_state_is_normal) {
    GameState state{};
    CHECK(modes::trap_speed_mode(state) == TrapSpeedMode::Normal);
    CHECK(!modes::assist_active(state));
}

TEST_CASE(modes_assist_toggle_flips_on_and_off) {
    GameState state{};
    modes::toggle_assist_mode(state);
    CHECK(state.assist_mode_enabled);
    CHECK(modes::assist_active(state));
    CHECK(modes::trap_speed_mode(state) == TrapSpeedMode::Assist);

    modes::toggle_assist_mode(state);
    CHECK(!state.assist_mode_enabled);
    CHECK(modes::trap_speed_mode(state) == TrapSpeedMode::Normal);
}

TEST_CASE(modes_hard_toggle_flips_on_and_off) {
    GameState state{};
    modes::toggle_hard_mode(state);
    CHECK(state.hard_mode_enabled);
    CHECK(modes::trap_speed_mode(state) == TrapSpeedMode::Hard);

    modes::toggle_hard_mode(state);
    CHECK(!state.hard_mode_enabled);
    CHECK(modes::trap_speed_mode(state) == TrapSpeedMode::Normal);
}

TEST_CASE(modes_assist_on_turns_hard_off) {
    GameState state{};
    state.hard_mode_enabled = true;
    modes::toggle_assist_mode(state);
    CHECK(state.assist_mode_enabled);
    CHECK(!state.hard_mode_enabled);
}

TEST_CASE(modes_hard_on_turns_assist_off) {
    GameState state{};
    state.assist_mode_enabled = true;
    modes::toggle_hard_mode(state);
    CHECK(state.hard_mode_enabled);
    CHECK(!state.assist_mode_enabled);
}

TEST_CASE(modes_turning_one_off_does_not_touch_the_other) {
    GameState state{};
    state.assist_mode_enabled = true;
    modes::toggle_assist_mode(state);  // assist on -> off
    CHECK(!state.assist_mode_enabled);
    CHECK(!state.hard_mode_enabled);

    state.hard_mode_enabled = true;
    modes::toggle_hard_mode(state);  // hard on -> off
    CHECK(!state.hard_mode_enabled);
    CHECK(!state.assist_mode_enabled);
}

TEST_CASE(modes_hard_wins_when_both_flags_are_set) {
    // Inconsistent save data (cannot be produced through the menu).
    GameState state{};
    state.hard_mode_enabled = true;
    state.assist_mode_enabled = true;
    CHECK(modes::trap_speed_mode(state) == TrapSpeedMode::Hard);
    CHECK(!modes::assist_active(state));
}

TEST_CASE(modes_toggling_never_leaves_both_flags_set) {
    GameState state{};
    for (int i = 0; i < 6; ++i) {
        modes::toggle_assist_mode(state);
        CHECK(!(state.assist_mode_enabled && state.hard_mode_enabled));
        modes::toggle_hard_mode(state);
        CHECK(!(state.assist_mode_enabled && state.hard_mode_enabled));
    }
}

TEST_CASE(modes_mode_toggles_do_not_touch_hard_mode_unlock) {
    GameState state{};
    modes::toggle_hard_mode(state);
    modes::toggle_assist_mode(state);
    CHECK(!state.hard_mode_unlocked);
}

TEST_CASE(modes_options_rows_without_hard_mode_unlocked) {
    CHECK_EQ(modes::assist_row(false), 2);
    CHECK_EQ(modes::options_row_count(false), 3);
}

TEST_CASE(modes_options_rows_with_hard_mode_unlocked) {
    // Hard Mode occupies row 2, Assist Mode moves down to row 3.
    CHECK_EQ(modes::assist_row(true), 3);
    CHECK_EQ(modes::options_row_count(true), 4);
}

TEST_CASE(modes_assist_row_is_always_the_last_row) {
    CHECK_EQ(modes::assist_row(false), modes::options_row_count(false) - 1);
    CHECK_EQ(modes::assist_row(true), modes::options_row_count(true) - 1);
}

TEST_CASE(modes_records_are_enabled_in_normal_and_hard_mode) {
    GameState state{};
    CHECK(modes::records_enabled(state));
    state.hard_mode_enabled = true;
    CHECK(modes::records_enabled(state));
}

TEST_CASE(modes_records_are_disabled_in_assist_mode) {
    GameState state{};
    modes::toggle_assist_mode(state);
    CHECK(!modes::records_enabled(state));
}

TEST_CASE(modes_records_come_back_when_assist_mode_is_turned_off) {
    GameState state{};
    modes::toggle_assist_mode(state);
    modes::toggle_assist_mode(state);
    CHECK(modes::records_enabled(state));
}

TEST_CASE(modes_records_follow_hard_mode_precedence_for_inconsistent_flags) {
    // Both flags set (cannot happen via the menu): Hard Mode wins, so the
    // run is not an assisted one and records are enabled.
    GameState state{};
    state.hard_mode_enabled = true;
    state.assist_mode_enabled = true;
    CHECK(modes::records_enabled(state));
}
