#include "test_framework.h"

#include "dash_trail.h"

namespace {

// Runs one Player::update() worth of trail bookkeeping while dashing.
auto dash_frame(DashTrail& trail) -> int {
    const int slot = trail.on_dash_frame();
    trail.tick();
    return slot;
}

auto active_count(const DashTrail& trail) -> int {
    int count = 0;
    for (int slot = 0; slot < DashTrail::SLOTS; ++slot) {
        if (trail.active(slot)) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST_CASE(dash_trail_starts_empty) {
    DashTrail trail;
    CHECK_EQ(active_count(trail), 0);
}

TEST_CASE(dash_trail_first_dash_frame_spawns_a_ghost) {
    DashTrail trail;
    trail.begin();
    CHECK(trail.on_dash_frame() >= 0);
}

TEST_CASE(dash_trail_spawns_on_every_second_dash_frame) {
    DashTrail trail;
    trail.begin();
    bool spawned[8];
    for (int frame = 0; frame < 8; ++frame) {
        spawned[frame] = dash_frame(trail) >= 0;
    }
    // Frames 0, 2, 4, 6 of an 8-frame dash.
    for (int frame = 0; frame < 8; ++frame) {
        CHECK_EQ(spawned[frame], frame % DashTrail::SPAWN_EVERY == 0);
    }
}

TEST_CASE(dash_trail_concurrent_ghosts_use_distinct_slots) {
    DashTrail trail;
    trail.begin();
    int first = dash_frame(trail);
    dash_frame(trail);
    int second = dash_frame(trail);
    CHECK(first >= 0 && second >= 0);
    CHECK(first != second);
}

TEST_CASE(dash_trail_ghost_lives_exactly_life_frames) {
    DashTrail trail;
    trail.begin();
    const int slot = trail.on_dash_frame();
    CHECK_EQ(trail.life(slot), DashTrail::LIFE_FRAMES);
    for (int frame = 0; frame < DashTrail::LIFE_FRAMES; ++frame) {
        CHECK(trail.active(slot));
        trail.tick();
    }
    CHECK(!trail.active(slot));
}

TEST_CASE(dash_trail_ghost_flickers_and_is_hidden_when_spawned) {
    DashTrail trail;
    trail.begin();
    const int slot = trail.on_dash_frame();
    // life 6 (just spawned): hidden, then 5 visible, 4 hidden, 3 visible, ...
    bool pattern[DashTrail::LIFE_FRAMES];
    for (int frame = 0; frame < DashTrail::LIFE_FRAMES; ++frame) {
        pattern[frame] = trail.visible(slot);
        trail.tick();
    }
    CHECK(!pattern[0]);
    CHECK(pattern[1]);
    CHECK(!pattern[2]);
    CHECK(pattern[3]);
    CHECK(!pattern[4]);
    CHECK(pattern[5]);
    CHECK(!trail.visible(slot));  // gone
}

TEST_CASE(dash_trail_never_needs_more_than_the_available_slots) {
    DashTrail trail;
    trail.begin();
    int most = 0;
    for (int frame = 0; frame < 8; ++frame) {
        dash_frame(trail);
        if (active_count(trail) > most) {
            most = active_count(trail);
        }
    }
    CHECK(most <= DashTrail::SLOTS);
    // Enough slots that no live ghost is ever replaced.
    CHECK(DashTrail::SLOTS * DashTrail::SPAWN_EVERY >= DashTrail::LIFE_FRAMES);
}

TEST_CASE(dash_trail_replaces_the_oldest_ghost_when_all_slots_are_busy) {
    DashTrail trail;
    // Spawn faster than ghosts die: begin() before every frame forces a
    // spawn each time.
    int first_slot = -1;
    for (int i = 0; i < DashTrail::SLOTS; ++i) {
        trail.begin();
        const int slot = trail.on_dash_frame();
        if (i == 0) {
            first_slot = slot;
        }
        trail.tick();
    }
    CHECK_EQ(active_count(trail), DashTrail::SLOTS);
    trail.begin();
    CHECK_EQ(trail.on_dash_frame(), first_slot);  // the oldest was reused
}

TEST_CASE(dash_trail_clear_removes_every_ghost) {
    DashTrail trail;
    trail.begin();
    dash_frame(trail);
    dash_frame(trail);
    dash_frame(trail);
    trail.clear();
    CHECK_EQ(active_count(trail), 0);
}

TEST_CASE(dash_trail_begin_makes_the_next_dash_spawn_at_once) {
    DashTrail trail;
    trail.begin();
    dash_frame(trail);  // spawns
    dash_frame(trail);  // cadence gap
    trail.begin();      // a new dash starts right away
    CHECK(trail.on_dash_frame() >= 0);
}

TEST_CASE(dash_trail_ghosts_keep_fading_after_the_dash_ends) {
    DashTrail trail;
    trail.begin();
    const int slot = dash_frame(trail);  // spawned and aged once
    CHECK(trail.active(slot));
    for (int frame = 0; frame < DashTrail::LIFE_FRAMES; ++frame) {
        trail.tick();  // no dash frames any more
    }
    CHECK(!trail.active(slot));
}
