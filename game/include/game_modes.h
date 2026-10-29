#pragma once

#include <cstdint>

#include "game_state.h"

// -----------------------------------------------------------------------------
// modes
//
// Pure decision logic for the Hard Mode / Assist Mode toggles, split out of
// LevelManager and PauseController so it has no Butano dependency and can be
// unit-tested on the host (see tests/host/test_game_modes.cpp). Only reads
// and writes GameState flags - the actual multipliers (Cfg::HardMode /
// Cfg::AssistMode) are applied by the callers.
// -----------------------------------------------------------------------------

namespace modes {

enum class TrapSpeedMode : uint8_t { Normal, Hard, Assist };

// Hard Mode wins if both flags are ever set (e.g. inconsistent save data) -
// the Options menu keeps them mutually exclusive.
inline auto trap_speed_mode(const GameState& state) -> TrapSpeedMode {
    if (state.hard_mode_enabled) {
        return TrapSpeedMode::Hard;
    }
    if (state.assist_mode_enabled) {
        return TrapSpeedMode::Assist;
    }
    return TrapSpeedMode::Normal;
}

inline auto assist_active(const GameState& state) -> bool {
    return trap_speed_mode(state) == TrapSpeedMode::Assist;
}

// Flips Assist Mode; turning it on turns Hard Mode off.
inline void toggle_assist_mode(GameState& state) {
    state.assist_mode_enabled = !state.assist_mode_enabled;
    if (state.assist_mode_enabled) {
        state.hard_mode_enabled = false;
    }
}

// Flips Hard Mode; turning it on turns Assist Mode off.
inline void toggle_hard_mode(GameState& state) {
    state.hard_mode_enabled = !state.hard_mode_enabled;
    if (state.hard_mode_enabled) {
        state.assist_mode_enabled = false;
    }
}

// Best times and no-death clears are only recorded outside Assist Mode: it is
// a comfort option, not a leaderboard run. (Progress and unlocks still count.)
inline auto records_enabled(const GameState& state) -> bool {
    return !assist_active(state);
}

// Mirror Mode (the levels flipped left-right, own records): a post-game mode,
// available with Hard Mode - from the moment the game has been finished. The
// toggle is remembered in the save, but only counts while it is unlocked.
inline auto mirror_active(const GameState& state) -> bool {
    return state.hard_mode_unlocked && state.mirror_mode_enabled;
}

// Whether a level can be picked in the world and level select: every level
// in Mirror Mode (the game has been finished), otherwise up to the furthest
// level reached.
inline auto level_unlocked(const GameState& state, int level_index) -> bool {
    return mirror_active(state) || level_index <= state.furthest_level;
}

// Flips Mirror Mode; does nothing while it is still locked.
inline void toggle_mirror_mode(GameState& state) {
    if (state.hard_mode_unlocked) {
        state.mirror_mode_enabled = !state.mirror_mode_enabled;
    }
}

// Options sub-menu rows: Music (0), SFX (1), [Hard Mode (2) once unlocked],
// Assist Mode, then Ghost (ghost_session.h).
inline auto assist_row(bool hard_unlocked) -> int {
    return hard_unlocked ? 3 : 2;
}

inline auto ghost_row(bool hard_unlocked) -> int {
    return assist_row(hard_unlocked) + 1;
}

inline auto options_row_count(bool hard_unlocked) -> int {
    return ghost_row(hard_unlocked) + 1;
}

}  // namespace modes
