#include "test_framework.h"

#include "crumble_logic.h"

using crumble::Event;
using crumble::Phase;
using crumble::State;

namespace {

// Runs idle frames (nobody on it, nobody in its box) and returns the frame on
// which `wanted` was reported (1-based), or -1.
auto frames_until(State& state, Event wanted, int limit) -> int {
    for (int frame = 1; frame <= limit; ++frame) {
        if (state.update(false, false) == wanted) {
            return frame;
        }
    }
    return -1;
}

}  // namespace

TEST_CASE(crumble_starts_solid_and_stays_that_way_without_the_duck) {
    State state(30, 120);
    for (int i = 0; i < 500; ++i) {
        CHECK(state.update(false, false) == Event::None);
    }
    CHECK(state.phase() == Phase::Solid);
    CHECK(state.solid());
    CHECK(state.visible());
}

TEST_CASE(crumble_first_step_starts_the_shaking) {
    State state(30, 120);
    CHECK(state.update(true, false) == Event::Started);
    CHECK(state.phase() == Phase::Shaking);
    CHECK(state.solid());
}

TEST_CASE(crumble_gives_way_crumble_frames_after_the_first_contact) {
    State state(30, 120);
    state.update(true, false);
    CHECK_EQ(frames_until(state, Event::Fell, 100), 30);
    CHECK(state.phase() == Phase::Falling);
    CHECK(!state.solid());
    CHECK(state.visible());
}

TEST_CASE(crumble_stepping_off_does_not_save_it) {
    State state(10, 0);
    state.update(true, false);
    state.update(false, false);  // the duck jumped off at once
    CHECK(state.phase() == Phase::Shaking);
    CHECK_EQ(frames_until(state, Event::Fell, 50), 9);
}

TEST_CASE(crumble_staying_on_it_does_not_restart_the_countdown) {
    State state(10, 0);
    state.update(true, false);
    int fell_on = -1;
    for (int frame = 1; frame <= 50; ++frame) {
        if (state.update(true, false) == Event::Fell) {
            fell_on = frame;
            break;
        }
    }
    CHECK_EQ(fell_on, 10);
}

TEST_CASE(crumble_one_frame_platform_falls_on_the_next_frame) {
    State state(1, 0);
    CHECK(state.update(true, false) == Event::Started);
    CHECK(state.update(true, false) == Event::Fell);
}

TEST_CASE(crumble_zero_frames_counts_as_one) {
    State state(0, 0);
    CHECK(state.update(true, false) == Event::Started);
    CHECK(state.update(true, false) == Event::Fell);
}

TEST_CASE(crumble_vanishes_after_the_fall_frames) {
    State state(5, 0);
    state.update(true, false);
    frames_until(state, Event::Fell, 20);
    CHECK_EQ(frames_until(state, Event::Vanished, 100), crumble::FALL_FRAMES);
    CHECK(state.phase() == Phase::Gone);
    CHECK(!state.solid());
    CHECK(!state.visible());
}

TEST_CASE(crumble_without_respawn_stays_gone_forever) {
    State state(5, 0);
    state.update(true, false);
    frames_until(state, Event::Vanished, 100);
    for (int i = 0; i < 1000; ++i) {
        CHECK(state.update(false, false) == Event::None);
    }
    CHECK(state.phase() == Phase::Gone);
}

TEST_CASE(crumble_respawns_after_respawn_frames) {
    State state(5, 100);
    state.update(true, false);
    frames_until(state, Event::Vanished, 100);
    CHECK_EQ(frames_until(state, Event::Respawned, 500), 100);
    CHECK(state.phase() == Phase::Solid);
    CHECK(state.solid());
    CHECK(state.visible());
}

TEST_CASE(crumble_waits_while_the_duck_is_inside_its_box) {
    State state(5, 10);
    state.update(true, false);
    frames_until(state, Event::Vanished, 100);
    for (int i = 0; i < 100; ++i) {
        CHECK(state.update(false, true) == Event::None);  // blocked
    }
    CHECK(state.phase() == Phase::Gone);
    CHECK(state.update(false, false) == Event::Respawned);
}

TEST_CASE(crumble_can_crumble_again_after_coming_back) {
    State state(5, 10);
    state.update(true, false);
    frames_until(state, Event::Respawned, 200);
    CHECK(state.update(true, false) == Event::Started);
    CHECK_EQ(frames_until(state, Event::Fell, 50), 5);
}

TEST_CASE(crumble_reset_puts_the_platform_back_from_any_phase) {
    for (int steps : {0, 3, 8, 25, 60}) {
        State state(5, 0);
        state.update(true, false);
        for (int i = 0; i < steps; ++i) {
            state.update(false, false);
        }
        state.reset();
        CHECK(state.phase() == Phase::Solid);
        CHECK(state.solid());
        CHECK(state.shake_offset() == 0);
        CHECK(state.fall_offset() == 0);
        // and it works as new: untouched until stepped on, then crumbles
        CHECK(state.update(false, false) == Event::None);
        CHECK(state.update(true, false) == Event::Started);
    }
}

TEST_CASE(crumble_shake_only_while_shaking_and_swings_both_ways) {
    State state(12, 0);
    CHECK_EQ(state.shake_offset(), 0);
    state.update(true, false);
    bool left = false;
    bool right = false;
    for (int i = 0; i < 11; ++i) {
        const int offset = state.shake_offset();
        CHECK(offset == -1 || offset == 1);
        left = left || offset < 0;
        right = right || offset > 0;
        state.update(false, false);
    }
    CHECK(left && right);
    state.update(false, false);  // gave way
    CHECK_EQ(state.shake_offset(), 0);
}

TEST_CASE(crumble_fall_offset_grows_while_falling_and_is_zero_otherwise) {
    State state(3, 0);
    CHECK_EQ(state.fall_offset(), 0);
    state.update(true, false);
    CHECK_EQ(state.fall_offset(), 0);
    frames_until(state, Event::Fell, 10);
    int previous = state.fall_offset();
    CHECK_EQ(previous, 0);
    for (int i = 1; i < crumble::FALL_FRAMES; ++i) {
        state.update(false, false);
        const int now = state.fall_offset();
        CHECK(now >= previous);
        previous = now;
    }
    CHECK(previous > 20);        // it clearly drops out of the picture
    state.update(false, false);  // vanished
    CHECK_EQ(state.fall_offset(), 0);
}

TEST_CASE(crumble_events_come_in_the_expected_order) {
    State state(4, 6);
    Event expected[] = {
        Event::Started, Event::Fell, Event::Vanished, Event::Respawned};
    int next = 0;
    bool standing = true;
    for (int frame = 0; frame < 200 && next < 4; ++frame) {
        const Event event = state.update(standing, false);
        standing = false;
        if (event != Event::None) {
            CHECK(event == expected[next]);
            ++next;
        }
    }
    CHECK_EQ(next, 4);
}
