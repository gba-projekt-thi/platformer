#include "test_framework.h"

#include "trigger_logic.h"

namespace {

// The pre-switch Trigger behavior, kept here as the reference for the
// backward-compatibility test below.
auto legacy_after_enter(bool default_on) -> bool {
    return !default_on;
}

}  // namespace

TEST_CASE(trigger_starts_in_its_default_state_in_every_mode) {
    for (TriggerMode mode :
         {TriggerMode::Latch, TriggerMode::Toggle, TriggerMode::Hold}) {
        CHECK(!TriggerState(false, mode).triggered());
        CHECK(TriggerState(true, mode).triggered());
    }
}

TEST_CASE(trigger_latch_matches_the_legacy_behavior) {
    // Backward compatibility: every existing level uses Latch (the default),
    // so entering must give exactly what the old `_triggered = !_default_on`
    // gave, for both default states, and leaving must change nothing.
    for (bool default_on : {false, true}) {
        TriggerState state(default_on, TriggerMode::Latch);
        state.on_enter();
        CHECK_EQ(state.triggered(), legacy_after_enter(default_on));
        state.on_exit();
        CHECK_EQ(state.triggered(), legacy_after_enter(default_on));
        state.reset();
        CHECK_EQ(state.triggered(), default_on);
    }
}

TEST_CASE(trigger_latch_stays_set_when_entered_again) {
    TriggerState state(false, TriggerMode::Latch);
    state.on_enter();
    state.on_exit();
    state.on_enter();
    CHECK(state.triggered());
}

TEST_CASE(trigger_toggle_flips_on_every_entry) {
    TriggerState state(false, TriggerMode::Toggle);
    state.on_enter();
    CHECK(state.triggered());
    state.on_enter();
    CHECK(!state.triggered());
    state.on_enter();
    CHECK(state.triggered());
}

TEST_CASE(trigger_toggle_ignores_exit) {
    TriggerState state(false, TriggerMode::Toggle);
    state.on_enter();
    state.on_exit();
    CHECK(state.triggered());
}

TEST_CASE(trigger_toggle_with_default_on_flips_to_off_first) {
    TriggerState state(true, TriggerMode::Toggle);
    state.on_enter();
    CHECK(!state.triggered());
    state.on_enter();
    CHECK(state.triggered());
}

TEST_CASE(trigger_hold_is_on_only_while_inside) {
    TriggerState state(false, TriggerMode::Hold);
    state.on_enter();
    CHECK(state.triggered());
    state.on_exit();
    CHECK(!state.triggered());
    state.on_enter();
    CHECK(state.triggered());
}

TEST_CASE(trigger_hold_with_default_on_is_off_only_while_inside) {
    TriggerState state(true, TriggerMode::Hold);
    state.on_enter();
    CHECK(!state.triggered());
    state.on_exit();
    CHECK(state.triggered());
}

TEST_CASE(trigger_reset_restores_the_default_state_in_every_mode) {
    for (TriggerMode mode :
         {TriggerMode::Latch, TriggerMode::Toggle, TriggerMode::Hold}) {
        for (bool default_on : {false, true}) {
            TriggerState state(default_on, mode);
            state.on_enter();
            state.reset();
            CHECK_EQ(state.triggered(), default_on);
        }
    }
}

TEST_CASE(trigger_reset_while_inside_a_toggle_starts_from_default_again) {
    // Death while standing on a switch: after the reset the next entry flips
    // from the default state, not from where the switch was left.
    TriggerState state(false, TriggerMode::Toggle);
    state.on_enter();  // on
    state.on_exit();
    state.on_enter();  // off
    state.on_exit();
    state.on_enter();  // on
    state.reset();     // death
    CHECK(!state.triggered());
    state.on_enter();
    CHECK(state.triggered());
}

TEST_CASE(trigger_restore_sets_latch_and_toggle_triggers) {
    for (TriggerMode mode : {TriggerMode::Latch, TriggerMode::Toggle}) {
        TriggerState state(false, mode);
        state.restore(true);
        CHECK(state.triggered());
        state.restore(false);
        CHECK(!state.triggered());
    }
}

TEST_CASE(trigger_restore_leaves_hold_triggers_alone) {
    // A pressure plate is only on while the duck stands on it; restoring it
    // "on" would leave it on forever (the exit never comes).
    TriggerState state(false, TriggerMode::Hold);
    state.restore(true);
    CHECK(!state.triggered());
}

TEST_CASE(trigger_reset_after_restore_returns_to_the_default) {
    TriggerState state(false, TriggerMode::Latch);
    state.restore(true);
    state.reset();
    CHECK(!state.triggered());
}

TEST_CASE(trigger_toggle_continues_from_a_restored_state) {
    TriggerState state(false, TriggerMode::Toggle);
    state.restore(true);
    state.on_enter();  // flips the restored "on" to off
    CHECK(!state.triggered());
}

// --- delay_frames and Timer triggers ---------------------------------------

namespace {

void run(TriggerState& state, int frames) {
    for (int i = 0; i < frames; ++i) {
        state.tick();
    }
}

}  // namespace

TEST_CASE(trigger_without_delay_follows_at_once_and_tick_changes_nothing) {
    for (TriggerMode mode :
         {TriggerMode::Latch, TriggerMode::Toggle, TriggerMode::Hold}) {
        TriggerState state(false, mode);
        state.on_enter();
        CHECK(state.triggered());
        CHECK(state.pressed());
        CHECK(!state.pending());
        run(state, 10);
        CHECK(state.triggered());
    }
}

TEST_CASE(trigger_delay_shows_the_press_at_once_but_the_effect_later) {
    TriggerState state(false, TriggerMode::Latch, 30);
    state.on_enter();
    CHECK(state.pressed());
    CHECK(!state.triggered());
    CHECK(state.pending());
    run(state, 29);
    CHECK(!state.triggered());
    state.tick();
    CHECK(state.triggered());
    CHECK(!state.pending());
}

TEST_CASE(trigger_delay_of_one_frame_takes_one_tick) {
    TriggerState state(false, TriggerMode::Latch, 1);
    state.on_enter();
    CHECK(!state.triggered());
    state.tick();
    CHECK(state.triggered());
}

TEST_CASE(trigger_delay_is_the_same_in_both_directions_for_a_hold_plate) {
    TriggerState state(false, TriggerMode::Hold, 5);
    state.on_enter();
    run(state, 5);
    CHECK(state.triggered());
    state.on_exit();
    CHECK(!state.pressed());
    CHECK(state.triggered());  // the trap still runs for 5 more frames
    run(state, 4);
    CHECK(state.triggered());
    state.tick();
    CHECK(!state.triggered());
}

TEST_CASE(trigger_delay_a_change_undone_in_time_never_shows) {
    TriggerState state(false, TriggerMode::Hold, 10);
    state.on_enter();
    run(state, 4);
    state.on_exit();  // left the plate before the delay was over
    CHECK(!state.pending());
    run(state, 20);
    CHECK(!state.triggered());
}

TEST_CASE(trigger_delay_toggle_flipped_twice_quickly_ends_where_it_began) {
    TriggerState state(false, TriggerMode::Toggle, 10);
    state.on_enter();
    run(state, 3);
    state.on_enter();
    run(state, 30);
    CHECK(!state.pressed());
    CHECK(!state.triggered());
}

TEST_CASE(trigger_delay_toggle_follows_each_press_with_the_delay) {
    TriggerState state(false, TriggerMode::Toggle, 4);
    state.on_enter();
    run(state, 4);
    CHECK(state.triggered());
    state.on_enter();
    CHECK(state.triggered());
    run(state, 4);
    CHECK(!state.triggered());
}

TEST_CASE(trigger_delay_with_default_on_counts_down_to_off) {
    TriggerState state(true, TriggerMode::Latch, 3);
    state.on_enter();
    CHECK(state.triggered());
    run(state, 3);
    CHECK(!state.triggered());
}

TEST_CASE(trigger_delay_reset_cancels_a_pending_change) {
    TriggerState state(false, TriggerMode::Latch, 30);
    state.on_enter();
    state.reset();
    CHECK(!state.pending());
    run(state, 60);
    CHECK(!state.triggered());
    CHECK(!state.pressed());
}

TEST_CASE(trigger_delay_restore_is_immediate_and_cancels_the_pending_change) {
    TriggerState state(false, TriggerMode::Latch, 30);
    state.restore(true);
    CHECK(state.triggered());
    CHECK(state.pressed());
    CHECK(!state.pending());

    TriggerState other(false, TriggerMode::Latch, 30);
    other.on_enter();
    other.restore(false);
    run(other, 60);
    CHECK(!other.triggered());
}

TEST_CASE(trigger_timer_fires_after_exactly_delay_frames) {
    TriggerState state(false, TriggerMode::Timer, 60);
    run(state, 59);
    CHECK(!state.triggered());
    state.tick();
    CHECK(state.triggered());
    CHECK(state.pressed());
}

TEST_CASE(trigger_timer_with_zero_delay_fires_on_the_first_tick) {
    TriggerState state(false, TriggerMode::Timer, 0);
    CHECK(!state.triggered());
    state.tick();
    CHECK(state.triggered());
}

TEST_CASE(trigger_timer_ignores_the_player) {
    TriggerState state(false, TriggerMode::Timer, 10);
    state.on_enter();
    state.on_exit();
    CHECK(!state.triggered());
    CHECK(!state.pressed());
}

TEST_CASE(trigger_timer_stays_on_until_reset_and_then_starts_over) {
    TriggerState state(false, TriggerMode::Timer, 5);
    run(state, 5);
    CHECK(state.triggered());
    run(state, 100);
    CHECK(state.triggered());
    state.reset();
    CHECK(!state.triggered());
    run(state, 4);
    CHECK(!state.triggered());
    state.tick();
    CHECK(state.triggered());
}

TEST_CASE(trigger_timer_with_default_on_switches_off) {
    TriggerState state(true, TriggerMode::Timer, 3);
    CHECK(state.triggered());
    run(state, 3);
    CHECK(!state.triggered());
}

TEST_CASE(trigger_timer_restore_keeps_a_fired_timer_fired) {
    TriggerState state(false, TriggerMode::Timer, 50);
    state.reset();
    state.restore(true);  // checkpoint reached after the timer had fired
    CHECK(state.triggered());
    run(state, 100);
    CHECK(state.triggered());
}

TEST_CASE(trigger_timer_restore_of_an_unfired_timer_counts_again) {
    TriggerState state(false, TriggerMode::Timer, 5);
    state.restore(false);
    run(state, 4);
    CHECK(!state.triggered());
    state.tick();
    CHECK(state.triggered());
}
