#include "test_framework.h"

#include <algorithm>
#include <string>
#include <vector>

#include "level_flow.h"

// The order of LevelManager's load, unload, update frame and respawn
// (level_flow.h), played through the real code with a Host that records every
// step and models what the game does with them: the death count, the death
// freeze, the pending respawn, the door and its boss lock.

namespace {

using level_flow::PauseAction;
using level_flow::Result;
using respawn::Cause;

struct FakeState {
    int16_t entry_level = -1;
    uint16_t entry_deaths = 0;
    int level = 3;
};

struct Host {
    std::vector<std::string> log;
    // The world
    unsigned player_deaths = 0;
    unsigned synced_deaths = 0;  // what the save has seen
    PauseAction pause_action = PauseAction::None;
    bool is_paused = false;
    bool trap_kills_this_frame = false;  // a trap in update_collisions()
    bool level_loaded = true;
    bool door = true;
    bool door_touched = false;
    bool door_is_locked = false;
    bool trapped_door = false;
    bool layout_differs = false;
    int freeze_frames = 3;
    // Real rules the flow relies on
    death_fx::Sequence fx;
    respawn::Pending pending;
    level_flow::Attempt att;
    // Bookkeeping for the invariants
    int platforms_alive = 0;
    int dynamic_views = 0;
    bool views_dangling = false;
    unsigned seen_at_door_check = 0;

    void note(const char* what) { log.emplace_back(what); }
    [[nodiscard]] auto count(const std::string& what) const -> int {
        return int(std::count(log.begin(), log.end(), what));
    }
    [[nodiscard]] auto at(const std::string& what, int nth = 0) const -> int {
        for (int i = 0; i < int(log.size()); ++i) {
            if (log[i] == what && nth-- == 0) {
                return i;
            }
        }
        return -1;
    }
    [[nodiscard]] auto has(const std::string& what) const -> bool {
        return at(what) >= 0;
    }
    // `first` is called, and before the first `second`
    [[nodiscard]] auto before(
        const std::string& first,
        const std::string& second) const -> bool {
        const int a = at(first);
        const int b = at(second);
        return a >= 0 && b >= 0 && a < b;
    }

#define STEP(name) \
    void name() { note(#name); }

    // load / unload
    STEP(reset_pause)
    STEP(reset_player_for_level)
    void clear_death_fx() {
        note("clear_death_fx");
        fx.clear();
    }
    STEP(cancel_player_death_fx)
    STEP(reset_shake)
    void enter_level_attempt() {
        note("enter_level_attempt");
        att.seen = player_deaths;
    }
    void cancel_pending_respawn() {
        note("cancel_pending_respawn");
        pending.cancel();
    }
    STEP(load_door)
    STEP(place_door)
    STEP(load_music)
    STEP(load_background)
    STEP(setup_camera)
    STEP(validate_level)
    void load_platforms() {
        note("load_platforms");
        platforms_alive = 2;
    }
    STEP(load_triggers)
    STEP(find_door_trap)
    STEP(forget_door_trap_in_checkpoints)
    void load_dynamic_platforms() {
        note("load_dynamic_platforms");
        dynamic_views = 1;
    }
    STEP(load_traps)
    STEP(load_checkpoints)
    void remember_level() {
        note("remember_level");
        level_loaded = true;
    }
    STEP(apply_attempt)
    STEP(sync_switched)
    STEP(release_scenery)
    void forget_level() {
        note("forget_level");
        level_loaded = false;
    }
    STEP(forget_boss)
    STEP(forget_door_trap)
    void clear_dynamic_platforms() {
        note("clear_dynamic_platforms");
        dynamic_views = 0;
    }
    STEP(clear_fleeing_platforms)
    void clear_platforms() {
        note("clear_platforms");
        // the non-owning views must be gone before what they point into
        views_dangling |= dynamic_views != 0;
        platforms_alive = 0;
    }
    STEP(clear_triggers)
    STEP(clear_traps)
    STEP(clear_checkpoints)
    STEP(forget_checkpoint_run)
    STEP(clear_resettables)

    // respawn
    STEP(reset_entities)
    [[nodiscard]] auto has_level() const -> bool { return level_loaded; }
    auto attempt() -> level_flow::Attempt& { return att; }
    [[nodiscard]] auto deaths() const -> unsigned { return player_deaths; }
    auto layout_changes(unsigned, unsigned) -> bool { return layout_differs; }
    STEP(cue_layout_change)
    STEP(reset_crumbling)
    STEP(reset_fleeing)
    STEP(respawn_triggers)
    STEP(snap_camera)
    auto request_respawn(Cause cause) -> bool {
        note("request_respawn");
        return pending.request(cause, fx.frozen());
    }
    auto advance_death_fx() -> death_fx::Event {
        note("advance_death_fx");
        return fx.advance();
    }
    STEP(end_player_death_fx)
    auto freeze_ended_respawn() -> bool { return pending.freeze_ended(); }

    // restart / deaths
    STEP(clear_checkpoint)
    STEP(note_restart)
    void kill_player() {
        note("kill_player");
        ++player_deaths;
    }
    auto sync_save() -> bool {
        note("sync_save");
        const bool saved = player_deaths != synced_deaths;
        synced_deaths = player_deaths;
        return saved;
    }
    STEP(force_save)
    void start_death_fx() {
        note("start_death_fx");
        fx.start(freeze_frames, 5);
    }
    STEP(begin_player_death_fx)

    // door
    [[nodiscard]] auto has_door() const -> bool { return door; }
    [[nodiscard]] auto door_trap_on() const -> bool { return trapped_door; }
    [[nodiscard]] auto door_reached() const -> bool { return door_touched; }
    [[nodiscard]] auto door_locked() const -> bool { return door_is_locked; }
    void clear_door_reached() {
        note("clear_door_reached");
        door_touched = false;
    }

    // one frame
    auto pause_update() -> PauseAction {
        note("pause_update");
        const PauseAction action = pause_action;
        pause_action = PauseAction::None;
        return action;
    }
    [[nodiscard]] auto paused() const -> bool { return is_paused; }
    [[nodiscard]] auto frozen() const -> bool { return fx.frozen(); }
    STEP(clear_dash_trail)
    STEP(count_frame)
    void update_collisions() {
        note("update_collisions");
        if (trap_kills_this_frame) {
            ++player_deaths;
            trap_kills_this_frame = false;
        }
    }
    STEP(propagate_triggers)
    STEP(sync_checkpoints)
    STEP(sync_crumbling)
    STEP(update_fleeing)
    STEP(sync_boss_door)
    STEP(feel_boss_hits)
    STEP(update_boss_music)
    STEP(flee_door)
    STEP(update_camera)
    STEP(apply_shake)
    STEP(sync_sprites)
    STEP(core_update)
#undef STEP
};

// Plays one frame; the log of the steps is in host.log.
auto frame(Host& h) -> Result {
    h.log.clear();
    return level_flow::update(h);
}

}  // namespace

// --- load -------------------------------------------------------------------

TEST_CASE(
    level_flow_load_creates_the_dynamic_platforms_after_platforms_and_triggers) {
    Host h;
    level_flow::load(h);
    CHECK(h.before("load_platforms", "load_dynamic_platforms"));
    CHECK(h.before("load_triggers", "load_dynamic_platforms"));
    CHECK(h.before("load_dynamic_platforms", "sync_switched"));
    CHECK(h.has("load_checkpoints"));
}

TEST_CASE(level_flow_load_applies_the_attempt_once_everything_exists) {
    Host h;
    level_flow::load(h);
    for (const char* step :
         {"load_platforms", "load_dynamic_platforms", "load_traps"}) {
        CHECK(h.before(step, "apply_attempt"));
    }
    CHECK(h.before("apply_attempt", "sync_switched"));
}

TEST_CASE(
    level_flow_load_finds_the_door_trap_after_the_triggers_and_before_the_checkpoints_forget_it) {
    Host h;
    level_flow::load(h);
    CHECK(h.before("load_triggers", "find_door_trap"));
    CHECK(h.before("find_door_trap", "forget_door_trap_in_checkpoints"));
}

TEST_CASE(level_flow_load_places_the_door_after_loading_it_and_the_entry) {
    Host h;
    level_flow::load(h);
    CHECK(h.before("load_door", "place_door"));
    // the door's spot depends on the deaths since the entry
    CHECK(h.before("enter_level_attempt", "place_door"));
}

TEST_CASE(
    level_flow_load_forgets_the_previous_runtime_state_and_death_feedback) {
    Host h;
    level_flow::load(h);
    for (const char* step :
         {"clear_death_fx", "cancel_player_death_fx", "reset_shake",
          "cancel_pending_respawn", "forget_door_trap",
          "clear_dynamic_platforms", "clear_checkpoints",
          "forget_checkpoint_run", "reset_pause"}) {
        CHECK(h.has(step));
    }
    CHECK(h.before("clear_death_fx", "enter_level_attempt"));
    CHECK(h.before("forget_door_trap", "load_platforms"));
    CHECK(!h.views_dangling);
}

TEST_CASE(level_flow_unload_leaves_no_feedback_or_runtime_state_behind) {
    Host h;
    level_flow::unload(h);
    for (const char* step :
         {"clear_death_fx", "cancel_player_death_fx", "reset_shake",
          "cancel_pending_respawn", "forget_door_trap",
          "clear_dynamic_platforms", "clear_checkpoints",
          "forget_checkpoint_run", "forget_level"}) {
        CHECK(h.has(step));
    }
    CHECK(!h.level_loaded);
    CHECK(!h.views_dangling);
}

TEST_CASE(level_flow_unload_cancels_a_respawn_that_waited_for_a_freeze) {
    Host h;
    h.fx.start(3, 0);
    h.pending.request(Cause::Death, true);
    level_flow::unload(h);
    CHECK(!h.fx.frozen());
    CHECK(!h.pending.freeze_ended());
}

TEST_CASE(level_flow_clear_removes_the_views_before_the_containers) {
    Host h;
    h.platforms_alive = 2;
    h.dynamic_views = 1;
    level_flow::clear_runtime_state(h);
    CHECK(!h.views_dangling);
    CHECK(h.before("clear_dynamic_platforms", "clear_platforms"));
}

// --- Attempt: the deaths of this level
// -----------------------------------------

TEST_CASE(level_flow_attempt_counts_the_deaths_of_the_level_from_its_entry) {
    FakeState state;
    level_flow::Attempt attempt;
    attempt.enter(state, 7);  // a fresh visit with 7 deaths so far
    CHECK_EQ(attempt.at_entry, 7u);
    CHECK_EQ(attempt.in_level(7), 0u);
    CHECK_EQ(attempt.in_level(9), 2u);
    CHECK_EQ(attempt.seen, 7u);
    CHECK_EQ(attempt.layout, 0u);
}

TEST_CASE(level_flow_attempt_the_entry_goes_into_the_save_for_continue) {
    FakeState state;
    level_flow::Attempt attempt;
    attempt.enter(state, 7);
    CHECK_EQ(int(state.entry_level), state.level);
    CHECK_EQ(int(state.entry_deaths), 7);

    // Title -> Continue: the same level, 2 deaths later, keeps its entry and
    // starts in the layout of its deaths (no cue for it).
    level_flow::Attempt again;
    again.enter(state, 9);
    CHECK_EQ(again.at_entry, 7u);
    CHECK_EQ(again.in_level(9), 2u);
    CHECK_EQ(again.layout, 2u);
    CHECK_EQ(again.seen, 9u);
    CHECK_EQ(int(state.entry_deaths), 7);
}

TEST_CASE(level_flow_attempt_another_level_is_a_new_first_attempt) {
    FakeState state;
    level_flow::Attempt attempt;
    attempt.enter(state, 7);
    state.level = 4;
    attempt.enter(state, 9);
    CHECK_EQ(attempt.at_entry, 9u);
    CHECK_EQ(attempt.layout, 0u);
    CHECK_EQ(int(state.entry_level), 4);
}

// --- one frame: order
// ---------------------------------------------------------

TEST_CASE(level_flow_a_normal_frame_syncs_after_the_collision_update) {
    Host h;
    CHECK(frame(h) == Result::None);
    for (const char* step :
         {"propagate_triggers", "sync_checkpoints", "sync_switched",
          "sync_crumbling", "update_fleeing", "sync_boss_door",
          "feel_boss_hits", "update_boss_music", "flee_door", "update_camera",
          "core_update"}) {
        CHECK(h.before("update_collisions", step));
    }
    CHECK(h.has("count_frame"));
    CHECK(h.before("count_frame", "update_collisions"));
}

TEST_CASE(
    level_flow_every_rendered_frame_applies_the_shake_then_syncs_sprites_then_updates_the_core) {
    Host h;
    frame(h);
    CHECK(h.before("apply_shake", "sync_sprites"));
    CHECK(h.before("sync_sprites", "core_update"));
    CHECK_EQ(h.count("core_update"), 1);
}

TEST_CASE(level_flow_the_camera_steps_before_rendering) {
    Host h;
    frame(h);
    CHECK(h.before("update_camera", "apply_shake"));
}

TEST_CASE(level_flow_a_trap_death_is_noticed_right_after_the_physics_step) {
    Host h;
    h.trap_kills_this_frame = true;
    h.log.clear();
    CHECK(level_flow::update(h) == Result::None);
    CHECK(h.before("update_collisions", "start_death_fx"));
    CHECK(h.before("start_death_fx", "core_update"));
    CHECK(h.has("begin_player_death_fx"));
    CHECK(h.fx.frozen());
}

TEST_CASE(level_flow_the_camera_stays_on_the_death_spot_on_the_death_frame) {
    Host h;
    h.trap_kills_this_frame = true;
    frame(h);
    CHECK(!h.has("update_camera"));
}

TEST_CASE(level_flow_a_death_starts_the_freeze_and_hides_the_duck_once) {
    Host h;
    h.trap_kills_this_frame = true;
    frame(h);
    CHECK_EQ(h.count("start_death_fx"), 1);
    CHECK_EQ(h.count("begin_player_death_fx"), 1);
    // The next frames see the same death: no second freeze.
    frame(h);
    CHECK_EQ(h.count("start_death_fx"), 0);
}

TEST_CASE(level_flow_a_quiet_frame_starts_no_death_feedback) {
    Host h;
    frame(h);
    CHECK(!h.has("start_death_fx"));
    CHECK(!h.has("begin_player_death_fx"));
}

// --- the death freeze
// ---------------------------------------------------------

TEST_CASE(level_flow_a_frozen_world_skips_the_simulation_and_the_level_clock) {
    Host h;
    h.trap_kills_this_frame = true;
    frame(h);  // the death frame: the freeze starts
    CHECK(h.fx.frozen());
    frame(h);
    CHECK(!h.has("update_collisions"));
    CHECK(!h.has("count_frame"));
    CHECK(!h.has("update_camera"));
    CHECK(h.has("core_update"));
    CHECK(h.has("advance_death_fx"));
}

TEST_CASE(level_flow_the_pause_check_comes_before_the_freeze) {
    Host h;
    h.fx.start(3, 0);
    h.is_paused = true;
    CHECK(frame(h) == Result::None);
    CHECK(h.has("pause_update"));
    // paused: nothing renders, the freeze does not run down
    CHECK(!h.has("core_update"));
    CHECK(!h.has("advance_death_fx"));
    CHECK(h.fx.frozen());
}

TEST_CASE(level_flow_a_paused_game_does_not_simulate) {
    Host h;
    h.is_paused = true;
    frame(h);
    CHECK(!h.has("update_collisions"));
    CHECK(!h.has("count_frame"));
}

TEST_CASE(level_flow_a_death_in_the_world_respawns_when_the_freeze_ends) {
    Host h;
    h.freeze_frames = 3;
    h.trap_kills_this_frame = true;
    frame(h);
    // the death frame: saved, but the next attempt waits for the freeze
    CHECK(!h.has("reset_entities"));
    CHECK_EQ(h.count("sync_save"), 1);
    frame(h);
    CHECK(!h.has("reset_entities"));
    frame(h);
    CHECK(h.has("reset_entities"));
    CHECK_EQ(h.count("snap_camera"), 1);
    // exactly once
    frame(h);
    CHECK(!h.has("reset_entities"));
}

TEST_CASE(level_flow_a_respawn_resets_entities_then_platforms_and_triggers) {
    Host h;
    h.fx.start(1, 0);
    h.pending.request(Cause::Death, true);
    frame(h);  // the freeze ends in this frame
    CHECK(h.before("reset_entities", "reset_crumbling"));
    CHECK(h.before("reset_entities", "reset_fleeing"));
    CHECK(h.before("reset_entities", "respawn_triggers"));
    CHECK(h.before("respawn_triggers", "snap_camera"));
    CHECK(h.before("place_door", "snap_camera"));
}

TEST_CASE(
    level_flow_a_respawn_applies_the_attempt_after_the_reset_and_before_the_platform_resets) {
    Host h;
    level_flow::finish_respawn(h);
    CHECK(h.before("reset_entities", "apply_attempt"));
    CHECK(h.before("apply_attempt", "load_dynamic_platforms"));
    CHECK(h.before("load_dynamic_platforms", "reset_crumbling"));
    CHECK(h.before("load_dynamic_platforms", "reset_fleeing"));
}

TEST_CASE(level_flow_a_respawn_places_the_door_again) {
    Host h;
    level_flow::finish_respawn(h);
    CHECK(h.has("place_door"));
}

TEST_CASE(
    level_flow_a_respawn_into_a_changed_layout_gives_the_cue_after_applying_it) {
    Host h;
    h.layout_differs = true;
    level_flow::finish_respawn(h);
    CHECK(h.before("apply_attempt", "cue_layout_change"));
    CHECK_EQ(h.count("cue_layout_change"), 1);
}

TEST_CASE(level_flow_a_respawn_into_the_same_layout_gives_no_cue) {
    Host h;
    level_flow::finish_respawn(h);
    CHECK(!h.has("cue_layout_change"));
}

TEST_CASE(level_flow_a_respawn_remembers_the_layout_it_set_up) {
    Host h;
    h.att.at_entry = 2;
    h.player_deaths = 5;
    level_flow::finish_respawn(h);
    CHECK_EQ(h.att.layout, 3u);
}

TEST_CASE(level_flow_a_respawn_without_a_level_applies_no_attempt) {
    Host h;
    h.level_loaded = false;
    level_flow::finish_respawn(h);
    CHECK(!h.has("apply_attempt"));
    CHECK(h.has("reset_entities"));
    CHECK(h.has("snap_camera"));
}

// --- Restart Level -----------------------------------------------------------

TEST_CASE(level_flow_restart_clears_the_checkpoint_before_the_death) {
    Host h;
    h.pause_action = PauseAction::Restart;
    frame(h);
    CHECK(h.before("clear_checkpoint", "kill_player"));
    CHECK(h.has("note_restart"));
}

TEST_CASE(level_flow_restart_is_exactly_one_death_without_feedback) {
    Host h;
    h.pause_action = PauseAction::Restart;
    h.player_deaths = 4;
    h.synced_deaths = 4;
    h.att.seen = 4;
    frame(h);
    CHECK_EQ(h.player_deaths, 5u);
    CHECK_EQ(h.count("kill_player"), 1);
    CHECK(!h.has("start_death_fx"));
    CHECK(!h.has("begin_player_death_fx"));
    CHECK(!h.fx.frozen());
    // saved right there: the end of the frame sets up no second attempt
    CHECK_EQ(h.count("reset_entities"), 1);
    CHECK_EQ(h.synced_deaths, 5u);
}

TEST_CASE(
    level_flow_restart_takes_and_saves_the_death_before_the_next_attempt_is_set_up) {
    Host h;
    h.pause_action = PauseAction::Restart;
    frame(h);
    CHECK(h.before("kill_player", "sync_save"));
    CHECK(h.before("sync_save", "reset_entities"));
}

TEST_CASE(level_flow_restart_sets_up_the_next_attempt_before_the_world_moves) {
    Host h;
    h.pause_action = PauseAction::Restart;
    frame(h);
    CHECK(h.before("kill_player", "reset_entities"));
    CHECK(h.before("reset_entities", "update_collisions"));
}

TEST_CASE(level_flow_restart_cancels_a_freeze_that_was_still_running) {
    Host h;
    h.fx.start(5, 0);
    h.pending.request(Cause::Death, true);
    h.pause_action = PauseAction::Restart;
    frame(h);
    CHECK(h.before("clear_death_fx", "kill_player"));
    CHECK(h.before("cancel_player_death_fx", "kill_player"));
    CHECK(h.before("cancel_pending_respawn", "kill_player"));
    CHECK(!h.fx.frozen());
    // the cancelled respawn does not run a second time later
    CHECK(!h.pending.freeze_ended());
    // and the world moves on in the same frame
    CHECK(h.has("update_collisions"));
}

TEST_CASE(level_flow_return_to_title_saves_and_leaves) {
    Host h;
    h.pause_action = PauseAction::ReturnToTitle;
    CHECK(frame(h) == Result::ReturnToTitle);
    CHECK(h.has("force_save"));
    CHECK(h.has("clear_dash_trail"));
    CHECK(!h.has("update_collisions"));
}

// --- the door
// -------------------------------------------------------------------

TEST_CASE(level_flow_a_reached_door_completes_the_level) {
    Host h;
    h.door_touched = true;
    CHECK(frame(h) == Result::LevelComplete);
    CHECK(h.has("clear_dash_trail"));
}

TEST_CASE(level_flow_a_locked_door_never_completes_the_level) {
    // The door's own shape still fires while a boss lives: the finish goes
    // through door_logic::on_touch, which knows the lock.
    Host h;
    h.door_touched = true;
    h.door_is_locked = true;
    CHECK(frame(h) == Result::None);
    for (int i = 0; i < 5; ++i) {
        CHECK(frame(h) == Result::None);
    }
}

TEST_CASE(level_flow_a_level_without_a_door_cannot_complete) {
    Host h;
    h.door = false;
    h.door_touched = true;
    CHECK(frame(h) == Result::None);
}

TEST_CASE(level_flow_a_trapped_door_kills_before_the_death_is_noticed) {
    Host h;
    h.door_touched = true;
    h.trapped_door = true;
    CHECK(frame(h) == Result::None);
    CHECK(h.before("update_collisions", "kill_player"));
    CHECK(h.before("kill_player", "start_death_fx"));
    // not a level completion
    CHECK_EQ(h.player_deaths, 1u);
    CHECK(h.fx.frozen());
}

TEST_CASE(level_flow_a_duck_a_trap_caught_on_the_trapped_door_dies_once) {
    Host h;
    h.door_touched = true;
    h.trapped_door = true;
    h.trap_kills_this_frame = true;
    frame(h);
    CHECK_EQ(h.player_deaths, 1u);
    CHECK_EQ(h.count("kill_player"), 0);
    CHECK_EQ(h.count("start_death_fx"), 1);
}

TEST_CASE(level_flow_an_untrapped_door_does_not_kill) {
    Host h;
    h.door_touched = false;
    h.trapped_door = true;  // the trigger is on, but the duck is not there
    frame(h);
    CHECK_EQ(h.player_deaths, 0u);
}

TEST_CASE(level_flow_load_gives_no_layout_cue) {
    Host h;
    h.layout_differs = true;
    level_flow::load(h);
    CHECK(!h.has("cue_layout_change"));
}
