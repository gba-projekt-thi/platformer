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

// Options sub-menu rows: Music (0), SFX (1), [Hard Mode (2) once unlocked],
// then Assist Mode.
inline auto assist_row(bool hard_unlocked) -> int {
    return hard_unlocked ? 3 : 2;
}

inline auto options_row_count(bool hard_unlocked) -> int {
    return hard_unlocked ? 4 : 3;
}

}  // namespace modes
