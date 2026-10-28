#include "test_framework.h"

#include "player_events.h"
#include "trigger_logic.h"

namespace {

// Each test starts from a clean singleton.
auto events() -> PlayerEvents& {
    PlayerEvents& e = PlayerEvents::instance();
    e.clear();
    return e;
}

}  // namespace

TEST_CASE(player_events_start_empty) {
    PlayerEvents& e = events();
    CHECK(!e.jumped());
    CHECK(!e.dashed());
}

TEST_CASE(player_events_notes_are_independent) {
    PlayerEvents& e = events();
    e.note_jump();
    CHECK(e.jumped());
    CHECK(!e.dashed());
    e.note_dash();
    CHECK(e.jumped());
    CHECK(e.dashed());
}

TEST_CASE(player_events_begin_frame_ends_the_previous_frames_events) {
    PlayerEvents& e = events();
    e.note_jump();
    e.note_dash();
    e.begin_frame();
    CHECK(!e.jumped());
    CHECK(!e.dashed());
}

TEST_CASE(player_events_an_event_survives_until_the_next_begin_frame) {
    // A trigger that updates before the player in the same frame still sees
    // the event of the previous frame once; one that updates after it sees it
    // in the frame it happened. Either way it is there until begin_frame().
    PlayerEvents& e = events();
    e.note_jump();
    CHECK(e.jumped());
    CHECK(e.jumped());
    e.begin_frame();
    CHECK(!e.jumped());
}

TEST_CASE(player_events_clear_drops_everything) {
    PlayerEvents& e = events();
    e.note_jump();
    e.note_dash();
    e.clear();
    CHECK(!e.jumped());
    CHECK(!e.dashed());
}

TEST_CASE(trigger_action_enter_never_fires_from_the_event_check) {
    for (bool inside : {false, true}) {
        CHECK(!action_fires(TriggerAction::Enter, inside, true, true));
    }
}

TEST_CASE(trigger_action_jump_needs_the_duck_inside_and_a_jump) {
    CHECK(action_fires(TriggerAction::Jump, true, true, false));
    CHECK(!action_fires(TriggerAction::Jump, false, true, false));
    CHECK(!action_fires(TriggerAction::Jump, true, false, false));
    // a dash is not a jump
    CHECK(!action_fires(TriggerAction::Jump, true, false, true));
}

TEST_CASE(trigger_action_dash_needs_the_duck_inside_and_a_dash) {
    CHECK(action_fires(TriggerAction::Dash, true, false, true));
    CHECK(!action_fires(TriggerAction::Dash, false, false, true));
    CHECK(!action_fires(TriggerAction::Dash, true, false, false));
    CHECK(!action_fires(TriggerAction::Dash, true, true, false));
}

TEST_CASE(trigger_action_fired_event_drives_the_trigger_state) {
    // What Trigger::update() does: a firing action is an "entry".
    TriggerState latch(false, TriggerMode::Latch);
    if (action_fires(TriggerAction::Jump, true, true, false)) {
        latch.on_enter();
    }
    CHECK(latch.triggered());

    TriggerState toggle(false, TriggerMode::Toggle);
    for (int jump = 0; jump < 3; ++jump) {
        if (action_fires(TriggerAction::Jump, true, true, false)) {
            toggle.on_enter();
        }
    }
    CHECK(toggle.triggered());  // on, off, on
}

TEST_CASE(trigger_action_jump_hold_plate_is_released_by_leaving) {
    TriggerState plate(false, TriggerMode::Hold);
    if (action_fires(TriggerAction::Jump, true, true, false)) {
        plate.on_enter();
    }
    CHECK(plate.triggered());
    plate.on_exit();
    CHECK(!plate.triggered());
}

TEST_CASE(trigger_action_jump_with_a_delay_follows_late) {
    TriggerState state(false, TriggerMode::Latch, 20);
    if (action_fires(TriggerAction::Jump, true, true, false)) {
        state.on_enter();
    }
    CHECK(state.pressed());
    CHECK(!state.triggered());
}
