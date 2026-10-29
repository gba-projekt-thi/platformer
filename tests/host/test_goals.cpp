#include "test_framework.h"

#include "game_state.h"
#include "goals.h"

TEST_CASE(time_goal_needs_a_clear_within_the_target) {
    CHECK(goals::time_goal_met(45 * 60, 45));
    CHECK(goals::time_goal_met(1, 45));
    CHECK(!goals::time_goal_met(45 * 60 + 1, 45));
}

TEST_CASE(time_goal_is_never_met_without_a_clear_or_a_target) {
    CHECK(!goals::time_goal_met(0, 45));
    CHECK(!goals::time_goal_met(100, 0));
    CHECK(!goals::time_goal_met(100, -5));
}

TEST_CASE(no_death_goal_reads_its_bit_of_any_mask) {
    CHECK(goals::no_death_goal_met(0x5, 0));
    CHECK(!goals::no_death_goal_met(0x5, 1));
    CHECK(goals::no_death_goal_met(0x5, 2));
    CHECK(!goals::no_death_goal_met(0xFFFFFFFFu, 32));
}

TEST_CASE(no_death_goal_agrees_with_the_save_state_badge) {
    GameState state;
    set_no_death_clear(state, 7);
    CHECK(goals::no_death_goal_met(state.no_death_clears, 7));
    CHECK(!goals::no_death_goal_met(state.no_death_clears, 6));
}
