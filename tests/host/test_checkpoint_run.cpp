#include "test_framework.h"

#include <vector>

#include "checkpoint_logic.h"

// CheckpointRun: what LevelManager does with the checkpoint flags and the
// triggers during one attempt of a level, with stand-ins for Checkpoint and
// Trigger.

namespace {

struct FakeCheckpoint {
    bool touched = false;
    bool active = false;

    // Like Checkpoint::take_hit(): reports a touch once.
    auto take_hit() -> bool {
        const bool hit = touched;
        touched = false;
        return hit;
    }
    void set_active(bool value) { active = value; }
};

struct FakeTrigger {
    bool on = false;
    [[nodiscard]] auto is_triggered() const -> bool { return on; }
    void restore(bool value) { on = value; }
};

struct Level {
    std::vector<FakeCheckpoint> flags = std::vector<FakeCheckpoint>(3);
    std::vector<FakeTrigger> triggers = std::vector<FakeTrigger>(4);
    CheckpointRun run;

    auto touch(int index) -> int {
        flags[index].touched = true;
        return run.sync(flags, triggers);
    }
};

}  // namespace

TEST_CASE(checkpoint_run_nothing_touched_nothing_reached) {
    Level level;
    CHECK_EQ(level.run.sync(level.flags, level.triggers), -1);
    CHECK(!level.run.has_active());
}

TEST_CASE(checkpoint_run_touching_a_flag_makes_it_the_only_active_one) {
    Level level;
    CHECK_EQ(level.touch(1), 1);
    CHECK(level.run.has_active());
    CHECK(!level.flags[0].active);
    CHECK(level.flags[1].active);
    CHECK(!level.flags[2].active);
}

TEST_CASE(checkpoint_run_a_later_flag_takes_over) {
    Level level;
    level.touch(0);
    CHECK_EQ(level.touch(2), 2);
    CHECK(!level.flags[0].active);
    CHECK(level.flags[2].active);
}

TEST_CASE(checkpoint_run_walking_back_to_an_earlier_flag_changes_nothing) {
    Level level;
    level.touch(2);
    CHECK_EQ(level.touch(0), -1);
    CHECK(level.flags[2].active);
    CHECK(!level.flags[0].active);
}

TEST_CASE(checkpoint_run_respawn_switches_the_snapshot_triggers_on_again) {
    Level level;
    level.triggers[1].on = true;
    level.triggers[3].on = true;
    level.touch(0);
    // A death resets every trigger ...
    for (FakeTrigger& trigger : level.triggers) {
        trigger.on = false;
    }
    // ... and the checkpoint brings back those that were on.
    level.run.restore(level.triggers);
    CHECK(!level.triggers[0].on);
    CHECK(level.triggers[1].on);
    CHECK(!level.triggers[2].on);
    CHECK(level.triggers[3].on);
}

TEST_CASE(checkpoint_run_snapshot_is_taken_when_the_flag_is_reached) {
    Level level;
    level.touch(0);
    level.triggers[2].on = true;   // switched on after the checkpoint
    level.triggers[2].on = false;  // death
    level.run.restore(level.triggers);
    CHECK(!level.triggers[2].on);
}

TEST_CASE(checkpoint_run_without_a_checkpoint_restore_does_nothing) {
    Level level;
    level.run.restore(level.triggers);
    for (const FakeTrigger& trigger : level.triggers) {
        CHECK(!trigger.on);
    }
}

TEST_CASE(checkpoint_run_clear_starts_the_level_over) {
    // Restart Level / level load.
    Level level;
    level.triggers[0].on = true;
    level.touch(1);
    level.run.clear(level.flags);
    CHECK(!level.run.has_active());
    for (const FakeCheckpoint& flag : level.flags) {
        CHECK(!flag.active);
    }
    level.triggers[0].on = false;
    level.run.restore(level.triggers);
    CHECK(!level.triggers[0].on);
    CHECK_EQ(level.touch(0), 0);  // every flag counts again
}

TEST_CASE(checkpoint_run_a_forgotten_trigger_is_not_switched_back_on) {
    // The trigger of a door that turns into a trap (DoorData::
    // trap_trigger_name): on when the flag is reached, it must not make the
    // door deadly on every attempt after it.
    Level level;
    level.run.forget_trigger(2);
    level.triggers[1].on = true;
    level.triggers[2].on = true;
    level.touch(0);
    for (auto& trigger : level.triggers) {
        trigger.on = false;  // the death reset
    }
    level.run.restore(level.triggers);
    CHECK(level.triggers[1].on);
    CHECK(!level.triggers[2].on);
}

TEST_CASE(checkpoint_run_forget_none_keeps_every_trigger) {
    Level level;
    level.run.forget_trigger(2);
    level.run.forget_trigger(CheckpointRun::NO_TRIGGER);  // the next level
    level.triggers[2].on = true;
    level.touch(0);
    level.triggers[2].on = false;
    level.run.restore(level.triggers);
    CHECK(level.triggers[2].on);
}
