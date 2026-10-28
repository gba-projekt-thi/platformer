#include "test_framework.h"

#include <vector>

#include "checkpoint_logic.h"
#include "trigger_logic.h"

// Delayed trap chains (TriggerData::follows): a following trigger ignores the
// duck and is pressed by its source instead. The frame model is the one
// LevelManager runs:
//   1. every trigger ticks (CollisionRegistry::update_all(), in array order)
//   2. the duck's overlaps press / release triggers (on_enter / on_exit)
//   3. TriggerChains::propagate() hands the sources' state to the followers

namespace {

// Stand-in for Trigger: the parts TriggerChains and CheckpointRun use.
struct ChainTrigger {
    TriggerState state;

    [[nodiscard]] auto is_triggered() const -> bool {
        return state.triggered();
    }
    void follow(bool source_on) { state.follow(source_on); }
    void restore(bool on) { state.restore(on); }
};

constexpr int NONE = -1;

struct Level {
    std::vector<ChainTrigger> triggers;
    TriggerChains chains;
    CheckpointRun run;
    int frame_count = 0;

    // One game frame; `enter` / `exit`: the trigger the duck steps into /
    // out of in this frame.
    void frame(int enter = NONE, int exit = NONE) {
        if (frame_count == 0) {
            chains.capture(triggers);  // the level load (_load_triggers())
        }
        ++frame_count;
        for (ChainTrigger& trigger : triggers) {
            trigger.state.tick();
        }
        if (enter != NONE) {
            triggers[enter].state.on_enter();
        }
        if (exit != NONE) {
            triggers[exit].state.on_exit();
        }
        chains.propagate(triggers);
    }

    void frames(int count) {
        for (int i = 0; i < count; ++i) {
            frame();
        }
    }

    // Death: what LevelManager::_finish_respawn() does.
    void respawn() {
        for (ChainTrigger& trigger : triggers) {
            trigger.state.reset();
        }
        chains.respawn(triggers, [this](auto& all) { run.restore(all); });
    }

    // Touches the level's only checkpoint flag.
    void reach_checkpoint() {
        struct Flag {
            bool touched = true;
            auto take_hit() -> bool {
                const bool hit = touched;
                touched = false;
                return hit;
            }
            void set_active(bool) {}
        };
        std::vector<Flag> flags(1);
        run.sync(flags, triggers);
    }

    [[nodiscard]] auto on(int index) const -> bool {
        return triggers[index].is_triggered();
    }
};

auto source(TriggerMode mode, unsigned delay = 0, bool default_on = false)
    -> ChainTrigger {
    return {TriggerState(default_on, mode, delay)};
}

auto follower(unsigned delay = 0, bool default_on = false) -> ChainTrigger {
    return {TriggerState(default_on, TriggerMode::Latch, delay, true)};
}

// The frame in which each trigger first turned on, for a duck that steps
// into the source (index `plate`) in frame 1. 0 = never.
auto first_on_frames(Level& level, int plate, int frames) -> std::vector<int> {
    std::vector<int> first(level.triggers.size(), 0);
    for (int f = 1; f <= frames; ++f) {
        level.frame(f == 1 ? plate : NONE);
        for (int i = 0; i < int(level.triggers.size()); ++i) {
            if (first[i] == 0 && level.on(i)) {
                first[i] = level.frame_count;
            }
        }
    }
    return first;
}

}  // namespace

// --- the follower itself -----------------------------------------------------

TEST_CASE(chain_follower_ignores_the_duck) {
    for (TriggerMode mode :
         {TriggerMode::Latch, TriggerMode::Toggle, TriggerMode::Hold,
          TriggerMode::Timer}) {
        TriggerState state(false, mode, 0, true);
        state.on_enter();
        CHECK(!state.pressed());
        state.tick();
        CHECK(!state.triggered());  // not even a Timer runs on its own
    }
}

TEST_CASE(chain_follower_is_pressed_by_its_source) {
    TriggerState state(false, TriggerMode::Latch, 0, true);
    state.follow(true);
    CHECK(state.triggered());
    state.follow(false);
    CHECK(!state.triggered());
}

TEST_CASE(chain_follower_with_default_on_switches_off) {
    TriggerState state(true, TriggerMode::Latch, 0, true);
    state.follow(true);
    CHECK(!state.triggered());
    state.follow(false);
    CHECK(state.triggered());
}

TEST_CASE(chain_follower_runs_its_delay_once_per_change) {
    // follow() is called every frame with the same value: that must not
    // restart the countdown.
    TriggerState state(false, TriggerMode::Latch, 3, true);
    for (int i = 0; i < 3; ++i) {
        state.follow(true);
        CHECK(!state.triggered());
        state.tick();
    }
    CHECK(state.triggered());
}

TEST_CASE(chain_follower_restores_even_with_a_hold_mode) {
    // A follower's own mode is ignored, so a checkpoint brings it back on.
    TriggerState state(false, TriggerMode::Hold, 0, true);
    state.restore(true);
    CHECK(state.triggered());
}

// --- latency -----------------------------------------------------------------

TEST_CASE(chain_each_hop_costs_one_frame_plus_the_followers_delay) {
    Level level;
    level.triggers = {source(TriggerMode::Latch), follower(), follower(40)};
    level.chains.link(1, 0);
    level.chains.link(2, 1);
    const std::vector<int> first = first_on_frames(level, 0, 100);
    CHECK_EQ(first[0], 1);
    CHECK_EQ(first[1], 2);       // 1 frame after the plate
    CHECK_EQ(first[2], 2 + 41);  // 1 + 40 frames after the first hop
}

TEST_CASE(chain_a_delayed_source_counts_from_its_triggered_state) {
    Level level;
    level.triggers = {source(TriggerMode::Latch, 10), follower()};
    level.chains.link(1, 0);
    const std::vector<int> first = first_on_frames(level, 0, 30);
    CHECK_EQ(first[0], 11);
    CHECK_EQ(first[1], 12);
}

TEST_CASE(chain_timing_does_not_depend_on_the_trigger_order) {
    // plate -> a -> b -> c, once listed in order and once backwards.
    Level forward;
    forward.triggers = {
        source(TriggerMode::Latch), follower(), follower(5), follower()};
    forward.chains.link(1, 0);
    forward.chains.link(2, 1);
    forward.chains.link(3, 2);

    Level backward;
    backward.triggers = {
        follower(), follower(5), follower(), source(TriggerMode::Latch)};
    backward.chains.link(2, 3);
    backward.chains.link(1, 2);
    backward.chains.link(0, 1);

    const std::vector<int> f = first_on_frames(forward, 0, 30);
    const std::vector<int> b = first_on_frames(backward, 3, 30);
    CHECK_EQ(f[0], b[3]);
    CHECK_EQ(f[1], b[2]);
    CHECK_EQ(f[2], b[1]);
    CHECK_EQ(f[3], b[0]);
    CHECK_EQ(f[3], 1 + 1 + 6 + 1);
}

TEST_CASE(chain_one_source_can_start_several_followers) {
    Level level;
    level.triggers = {source(TriggerMode::Latch), follower(), follower(2)};
    level.chains.link(1, 0);
    level.chains.link(2, 0);
    const std::vector<int> first = first_on_frames(level, 0, 10);
    CHECK_EQ(first[1], 2);
    CHECK_EQ(first[2], 4);
}

// --- sources that start on ---------------------------------------------------

// A follower follows whether its source was ACTIVATED (triggered() differs
// from the source's default_on), not the source's raw state.

TEST_CASE(chain_follower_of_a_default_on_source_stays_quiet_at_load) {
    Level level;
    level.triggers = {source(TriggerMode::Latch, 0, true), follower(3)};
    level.chains.link(1, 0);
    for (int i = 0; i < 30; ++i) {
        level.frame();
        CHECK(level.on(0));
        CHECK(!level.on(1));
    }
}

TEST_CASE(chain_follower_fires_after_a_default_on_source_is_activated) {
    Level level;
    level.triggers = {source(TriggerMode::Latch, 0, true), follower(3)};
    level.chains.link(1, 0);
    level.frames(5);
    level.frame(0);  // onto the plate: the source switches off
    CHECK(!level.on(0));
    level.frames(3);
    CHECK(!level.on(1));
    level.frame();
    CHECK(level.on(1));  // 1 + 3 frames after the plate
    level.frames(30);
    CHECK(level.on(1));
}

TEST_CASE(chain_follower_releases_when_a_default_on_source_returns) {
    Level level;
    level.triggers = {source(TriggerMode::Hold, 0, true), follower(2)};
    level.chains.link(1, 0);
    level.frame(0);  // onto the plate
    level.frames(5);
    CHECK(level.on(1));
    level.frame(NONE, 0);  // off the plate: the source is back to default
    CHECK(level.on(0));
    level.frames(2);
    CHECK(level.on(1));
    level.frame();
    CHECK(!level.on(1));
}

TEST_CASE(chain_a_cycle_with_a_default_on_follower_never_fires) {
    // Nobody in a cycle is activated at load, so nothing starts it, whatever
    // the followers' default_on.
    Level level;
    level.triggers = {follower(), follower(0, true)};
    level.chains.link(0, 1);
    level.chains.link(1, 0);
    level.frames(10);
    CHECK(!level.on(0));
    CHECK(level.on(1));
}

// --- sources that turn off again --------------------------------------------

TEST_CASE(chain_follows_a_hold_plate_off_again) {
    Level level;
    level.triggers = {source(TriggerMode::Hold), follower(3)};
    level.chains.link(1, 0);
    level.frame(0);  // onto the plate
    level.frames(4);
    CHECK(level.on(1));
    level.frame(NONE, 0);  // off the plate
    CHECK(!level.on(0));
    level.frames(3);
    CHECK(level.on(1));  // 1 + 3 frames to follow
    level.frame();
    CHECK(!level.on(1));
}

TEST_CASE(chain_follows_a_toggle_lever_back_and_forth) {
    Level level;
    level.triggers = {source(TriggerMode::Toggle), follower()};
    level.chains.link(1, 0);
    level.frame(0);
    level.frame();
    CHECK(level.on(1));
    level.frame(0);  // flipped back
    CHECK(level.on(1));
    level.frame();
    CHECK(!level.on(1));
}

TEST_CASE(chain_a_source_undone_within_the_delay_never_shows) {
    Level level;
    level.triggers = {source(TriggerMode::Toggle), follower(10)};
    level.chains.link(1, 0);
    level.frame(0);
    level.frames(4);
    level.frame(0);  // lever flipped back in time
    level.frames(30);
    CHECK(!level.on(1));
}

TEST_CASE(chain_a_cycle_never_fires_and_never_hangs) {
    Level level;
    level.triggers = {follower(), follower()};
    level.chains.link(0, 1);
    level.chains.link(1, 0);
    level.frames(10);
    CHECK(!level.on(0));
    CHECK(!level.on(1));
}

TEST_CASE(chain_self_and_out_of_range_links_are_ignored) {
    Level level;
    level.triggers = {follower(), source(TriggerMode::Latch)};
    level.chains.link(0, 0);
    level.chains.link(0, 99);
    level.chains.link(-1, 1);
    level.chains.link(TriggerChains::MAX_TRIGGERS, 1);
    level.frame(1);
    level.frames(3);
    CHECK(!level.on(0));
}

// --- death and checkpoints ---------------------------------------------------

TEST_CASE(chain_death_resets_the_whole_chain) {
    Level level;
    level.triggers = {source(TriggerMode::Latch), follower(), follower(5)};
    level.chains.link(1, 0);
    level.chains.link(2, 1);
    level.frame(0);
    level.frames(20);
    CHECK(level.on(2));
    level.respawn();
    for (int i = 0; i < 30; ++i) {
        level.frame();
        CHECK(!level.on(0));
        CHECK(!level.on(1));
        CHECK(!level.on(2));
    }
}

TEST_CASE(chain_death_cancels_a_pending_hop) {
    Level level;
    level.triggers = {source(TriggerMode::Latch), follower(20)};
    level.chains.link(1, 0);
    level.frame(0);
    level.frames(5);
    level.respawn();
    level.frames(40);
    CHECK(!level.on(1));
}

TEST_CASE(chain_fully_fired_before_the_checkpoint_stays_fired) {
    Level level;
    level.triggers = {source(TriggerMode::Latch), follower(5), follower(5)};
    level.chains.link(1, 0);
    level.chains.link(2, 1);
    level.frame(0);
    level.frames(20);
    level.reach_checkpoint();
    level.respawn();
    CHECK(level.on(0));
    CHECK(level.on(1));
    CHECK(level.on(2));
    for (int i = 0; i < 30; ++i) {
        level.frame();
        CHECK(level.on(1));
        CHECK(level.on(2));
    }
}

TEST_CASE(chain_pending_at_the_checkpoint_runs_again_from_the_respawn) {
    // The plate was on, the follower still counting: after the respawn the
    // follower is off and its hop starts over (1 + delay frames).
    Level level;
    level.triggers = {source(TriggerMode::Latch), follower(20)};
    level.chains.link(1, 0);
    level.frame(0);
    level.frames(5);
    level.reach_checkpoint();
    level.respawn();
    CHECK(level.on(0));
    CHECK(!level.on(1));
    level.frames(20);
    CHECK(!level.on(1));
    level.frame();
    CHECK(level.on(1));
}

TEST_CASE(chain_follower_of_a_hold_plate_turns_off_after_a_checkpoint) {
    // Hold plates are not restored (the duck is no longer on them), so a
    // follower that was on goes off again one frame after the respawn.
    Level level;
    level.triggers = {source(TriggerMode::Hold), follower()};
    level.chains.link(1, 0);
    level.frame(0);
    level.frame();
    level.reach_checkpoint();
    level.respawn();
    CHECK(level.on(1));
    level.frame();
    CHECK(!level.on(1));
}
