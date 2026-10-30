#include "test_framework.h"

#include <vector>

#include "checkpoint_system.h"

// CheckpointSystem: the flags of a level and where the duck respawns, with
// stand-ins for Checkpoint, Trigger and Player.

namespace {

// The system keeps the flags to itself, so what a test does to them (a touch)
// and sees of them (active) goes through these tables, by index.
bool g_touched[4];
bool g_active[4];

struct FakeCheckpoint {
    struct Pos {
        int x;
        int y;
    };
    FakeCheckpoint(int x, int y) : pos{x, y}, index(count++) {}
    Pos pos;
    int index;
    static inline int count = 0;

    auto take_hit() -> bool {
        const bool hit = g_touched[index];
        g_touched[index] = false;
        return hit;
    }
    void set_active(bool value) { g_active[index] = value; }
};

struct FakeTrigger {
    bool on = false;
    [[nodiscard]] auto is_triggered() const -> bool { return on; }
    void restore(bool value) { on = value; }
};

struct FakePlayer {
    int spawn_x = -1;
    int spawn_y = -1;
    int calls = 0;
    void set_spawn_point(int x, int y) {
        spawn_x = x;
        spawn_y = y;
        ++calls;
    }
};

struct Point {
    int x;
    int y;
};
struct FakeLevel {
    int checkpoint_count = 0;
    Point checkpoints[4] = {};
};

using System = CheckpointSystem<std::vector<FakeCheckpoint>, int>;

struct Fixture {
    System system;
    FakeLevel level;
    std::vector<FakeTrigger> triggers = std::vector<FakeTrigger>(2);
    FakePlayer player;
    Fixture() {
        reset();
        level.checkpoint_count = 3;
        level.checkpoints[0] = {10, 20};
        level.checkpoints[1] = {30, 40};
        level.checkpoints[2] = {50, 60};
        system.set_level_spawn(1, 2);
        system.load(level);
    }
    static void reset() {
        FakeCheckpoint::count = 0;
        for (int i = 0; i < 4; ++i) {
            g_touched[i] = false;
            g_active[i] = true;
        }
    }
};

}  // namespace

TEST_CASE(checkpoint_system_load_makes_one_inactive_flag_per_entry) {
    Fixture f;
    CHECK(!g_active[0] && !g_active[1] && !g_active[2]);
    CHECK(!f.system.has_active());
}

TEST_CASE(checkpoint_system_nothing_touched_keeps_the_spawn) {
    Fixture f;
    CHECK(!f.system.sync(f.triggers, f.player));
    CHECK_EQ(f.player.calls, 0);
}

TEST_CASE(checkpoint_system_a_reached_flag_becomes_the_spawn_point) {
    Fixture f;
    g_touched[1] = true;
    CHECK(f.system.sync(f.triggers, f.player));
    CHECK_EQ(f.player.spawn_x, 30);
    CHECK_EQ(f.player.spawn_y, 40);
    CHECK(f.system.has_active());
    CHECK(!g_active[0] && g_active[1] && !g_active[2]);
}

TEST_CASE(checkpoint_system_an_earlier_flag_does_not_take_the_spawn_back) {
    Fixture f;
    g_touched[2] = true;
    f.system.sync(f.triggers, f.player);
    g_touched[0] = true;
    CHECK(!f.system.sync(f.triggers, f.player));
    CHECK_EQ(f.player.spawn_x, 50);
    CHECK_EQ(f.player.calls, 1);
}

TEST_CASE(checkpoint_system_clear_falls_back_to_the_level_spawn) {
    Fixture f;
    g_touched[1] = true;
    f.system.sync(f.triggers, f.player);
    f.system.clear(f.player);
    CHECK_EQ(f.player.spawn_x, 1);
    CHECK_EQ(f.player.spawn_y, 2);
    CHECK(!f.system.has_active());
    CHECK(!g_active[1]);
}

TEST_CASE(checkpoint_system_a_new_level_spawn_is_what_clear_returns_to) {
    Fixture f;
    f.system.set_level_spawn(7, 8);
    f.system.clear(f.player);
    CHECK_EQ(f.player.spawn_x, 7);
    CHECK_EQ(f.player.spawn_y, 8);
}

TEST_CASE(checkpoint_system_restores_the_triggers_that_were_on_at_the_flag) {
    Fixture f;
    f.triggers[1].on = true;
    g_touched[0] = true;
    f.system.sync(f.triggers, f.player);
    f.triggers[1].on = false;
    f.system.restore_triggers(f.triggers);
    CHECK(f.triggers[1].on);
    CHECK(!f.triggers[0].on);
}

TEST_CASE(checkpoint_system_restores_nothing_without_an_active_flag) {
    Fixture f;
    f.triggers[0].on = true;
    f.system.restore_triggers(f.triggers);
    f.triggers[0].on = false;
    f.system.restore_triggers(f.triggers);
    CHECK(!f.triggers[0].on);
}

TEST_CASE(checkpoint_system_forgotten_trigger_is_not_restored) {
    Fixture f;
    f.system.forget_trigger(1);
    f.triggers[1].on = true;
    g_touched[0] = true;
    f.system.sync(f.triggers, f.player);
    f.triggers[1].on = false;
    f.system.restore_triggers(f.triggers);
    CHECK(!f.triggers[1].on);
}

TEST_CASE(checkpoint_system_unload_steps_drop_flags_and_run) {
    Fixture f;
    g_touched[0] = true;
    f.system.sync(f.triggers, f.player);
    f.system.clear_flags();
    f.system.forget_run();
    CHECK(!f.system.has_active());
    FakeCheckpoint::count = 0;
    f.system.load(f.level);
    g_touched[0] = true;
    CHECK(f.system.sync(f.triggers, f.player));
}
