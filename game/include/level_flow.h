#pragma once

#include <cstdint>

#include "attempt_variant.h"
#include "cfg_death_fx.h"
#include "death_fx.h"
#include "door_logic.h"
#include "respawn_logic.h"

// -----------------------------------------------------------------------------
// level_flow
//
// The order in which LevelManager does things: load(), unload(), one update()
// frame, a respawn. The steps themselves need Butano (sprites, collisions,
// sound); the ORDER does not, and a step in the wrong place or a dropped step
// compiles, links and silently breaks the game. So the order lives here, as
// templates over a `Host` that offers one function per step. LevelManager is
// the real Host (LevelManager::FlowOps in level_manager.cpp, it just forwards
// to the private steps); tests/host/test_level_flow.cpp brings a Host that
// records what was called and models the death count, the freeze and the
// pending respawn, and plays frames through the very same code.
// -----------------------------------------------------------------------------

namespace level_flow {

enum class PauseAction : uint8_t { None, Restart, ReturnToTitle };
enum class Result : uint8_t { None, LevelComplete, ReturnToTitle };

// Death counts the level manager keeps track of (Level Devil, death feedback).
struct Attempt {
    // The run's death total the manager has reacted to (Restart Level takes
    // its own death at once, so it gets no feedback).
    unsigned seen = 0;
    // The run's death total when the level was entered: what in_level()
    // counts from.
    unsigned at_entry = 0;
    // Deaths in the level the current layout (death windows, door spot) was
    // set up for: a respawn compares the next one with it.
    unsigned layout = 0;

    [[nodiscard]] auto in_level(unsigned run_deaths) const -> unsigned {
        return attempt_variant::deaths_in_level(run_deaths, at_entry);
    }

    // Level load. A level the save was already in (Title -> Continue) carries
    // on with its deaths, so its death windows and door spot stay where they
    // were; the entry is written back to the save (`state`, saved with the
    // next death).
    template <typename State>
    void enter(State& state, unsigned run_deaths) {
        seen = run_deaths;
        at_entry = attempt_variant::entry_deaths(
            state.entry_level, state.entry_deaths, state.level, run_deaths);
        state.entry_level = state.level;
        state.entry_deaths =
            static_cast<decltype(state.entry_deaths)>(at_entry);
        layout = in_level(run_deaths);
    }
};

// Nothing of an earlier level's death feedback carries over.
template <typename Host>
void clear_death_feedback(Host& h) {
    h.clear_death_fx();
    h.cancel_player_death_fx();
    h.reset_shake();
}

// Views go before the containers they point into.
template <typename Host>
void clear_runtime_state(Host& h) {
    h.forget_boss();
    h.forget_door_trap();
    h.clear_dynamic_platforms();
    h.clear_fleeing_platforms();
    h.clear_platforms();
    h.clear_triggers();
    h.clear_traps();
    h.clear_checkpoints();
    h.forget_checkpoint_run();
    h.clear_resettables();
}

template <typename Host>
void load(Host& h) {
    h.reset_pause();
    h.reset_player_for_level();
    clear_death_feedback(h);
    h.enter_level_attempt();
    h.cancel_pending_respawn();
    h.load_door();
    h.place_door();
    h.load_music();
    h.load_background();
    h.setup_camera();

    clear_runtime_state(h);
    h.validate_level();

    h.load_platforms();
    h.load_triggers();
    // The door's trap trigger is looked up once the triggers exist, and a
    // checkpoint must never keep it on (restored after a death, it would make
    // the door deadly for good).
    h.find_door_trap();
    h.forget_door_trap_in_checkpoints();
    h.load_dynamic_platforms();
    h.load_traps();
    h.load_checkpoints();
    h.remember_level();
    h.apply_attempt();
    h.sync_switched();
}

template <typename Host>
void unload(Host& h) {
    clear_death_feedback(h);
    h.cancel_pending_respawn();
    h.release_scenery();
    clear_runtime_state(h);
    h.forget_level();
}

// A frame with the shake applied; the HUD and the background stay put.
template <typename Host>
void render_frame(Host& h) {
    h.apply_shake(Cfg::DeathFx::SHAKE_AMPLITUDE);
    h.sync_sprites();
    h.core_update();
}

// After a death: re-arms traps and triggers, applies this attempt's layout,
// cuts the camera to the duck.
template <typename Host>
void finish_respawn(Host& h) {
    h.reset_entities();
    h.place_door();
    // Level Devil: this attempt's traps and platforms, then the dynamic
    // platforms that are there start over (the resets below put them back).
    // Compared with the layout that was there, not with deaths - 1: Restart
    // Level during a death freeze skips an attempt.
    bool layout_changed = false;
    if (h.has_level()) {
        Attempt& attempt = h.attempt();
        const unsigned deaths = attempt.in_level(h.deaths());
        layout_changed = h.layout_changes(attempt.layout, deaths);
        attempt.layout = deaths;
        h.apply_attempt();
        h.load_dynamic_platforms();
    }
    if (layout_changed) {
        h.cue_layout_change(Cfg::DeathFx::LAYOUT_SHAKE_FRAMES);
    }
    h.reset_crumbling();
    h.reset_fleeing();
    // Nothing from before the death may reach a follower: the chain starts
    // from the reset (or checkpoint-restored) triggers.
    h.respawn_triggers();
    h.snap_camera();
}

// A replay starts in the layout of `deaths` deaths in the level: no cue for
// it, whatever the layout before was.
template <typename Host>
void start_replay_at(Host& h, unsigned deaths) {
    h.set_player_deaths_in_level(deaths);
    h.attempt().seen = h.deaths();
    h.attempt().layout = deaths;
    finish_respawn(h);
}

// A death the world caused waits for the end of its freeze; Restart Level
// sets up the next attempt at once (respawn_logic.h).
template <typename Host>
void respawn(Host& h, respawn::Cause cause) {
    if (h.request_respawn(cause)) {
        finish_respawn(h);
    }
}

template <typename Host>
void advance_death_fx(Host& h) {
    if (h.advance_death_fx() != death_fx::Event::FreezeEnded) {
        return;
    }
    h.end_player_death_fx();
    if (h.freeze_ended_respawn()) {
        finish_respawn(h);
    }
}

// Restart Level is a death on purpose, without feedback: taken and saved
// right here, and the next attempt's layout is there before the physics step
// runs with the duck at the spawn.
template <typename Host>
void restart_level(Host& h) {
    // From the level's spawn point, not from a checkpoint.
    h.clear_checkpoint();
    h.note_restart();
    // Pausing during a death freeze and restarting must not leave the rest
    // of that freeze (or its ghost) behind.
    h.clear_death_fx();
    h.cancel_player_death_fx();
    h.cancel_pending_respawn();
    h.kill_player();
    h.attempt().seen = h.deaths();
    h.sync_save();
    respawn(h, respawn::Cause::Restart);
}

// A death the world caused (this frame, after the physics step): freeze,
// shake, ghost.
template <typename Host>
void check_player_died(Host& h) {
    const unsigned deaths = h.deaths();
    if (deaths == h.attempt().seen) {
        return;
    }
    h.attempt().seen = deaths;
    h.start_death_fx(Cfg::DeathFx::HITSTOP_FRAMES, Cfg::DeathFx::SHAKE_FRAMES);
    h.begin_player_death_fx();
}

// The door that turns into a trap kills like a trap does - right after the
// collision update, so this frame's death starts the death feedback.
template <typename Host>
void check_door_trap(Host& h) {
    if (!h.has_door()) {
        return;
    }
    // check_player_died() has not seen this frame's deaths yet: a trap that
    // caught the duck on the trapped door is its only death.
    const bool trapped = h.door_trap_on();
    const bool died = h.deaths() != h.attempt().seen;
    const door_logic::Touch touch =
        door_logic::on_touch(h.door_reached(), h.door_locked(), trapped, died);
    if (touch == door_logic::Touch::Kill || (trapped && died)) {
        h.clear_door_reached();
    }
    if (touch == door_logic::Touch::Kill) {
        h.kill_player();
    }
}

template <typename Host>
auto update(Host& h) -> Result {
    switch (h.pause_update()) {
        case PauseAction::Restart:
            restart_level(h);
            break;
        case PauseAction::ReturnToTitle:
            // Persist progress before leaving to the title screen.
            h.force_save();
            h.clear_dash_trail();
            return Result::ReturnToTitle;
        case PauseAction::None:
        default:
            break;
    }

    if (h.paused()) {
        return Result::None;
    }

    // Death freeze: no physics, no traps, no camera movement, no level clock.
    if (h.frozen()) {
        render_frame(h);
        advance_death_fx(h);
        return Result::None;
    }

    // Level clock: counts frames the level has actually been playable.
    h.count_frame();

    // Physics.
    h.update_collisions();
    h.propagate_triggers();
    h.sync_checkpoints();
    h.sync_switched();
    h.sync_crumbling();
    h.update_fleeing();
    h.sync_boss_door();
    h.feel_boss_hits();
    h.update_boss_music();
    h.flee_door();
    check_door_trap(h);
    check_player_died(h);

    // On the death frame the camera stays where the duck died.
    if (!h.frozen()) {
        h.update_camera();
    }

    render_frame(h);
    advance_death_fx(h);

    // The door's own shape fires while it is locked: the finish goes through
    // on_touch, which knows the boss lock.
    if (h.has_door() &&
        door_logic::on_touch(h.door_reached(), h.door_locked(), false) ==
            door_logic::Touch::Finish) {
        h.clear_dash_trail();
        return Result::LevelComplete;
    }

    // A death the world caused was saved: set up the next attempt.
    if (h.sync_save()) {
        respawn(h, respawn::Cause::Death);
    }
    return Result::None;
}

}  // namespace level_flow
