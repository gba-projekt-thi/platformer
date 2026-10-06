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
