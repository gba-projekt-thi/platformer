#pragma once

#include <cstdint>

#include "game_state.h"

// -----------------------------------------------------------------------------
// record_clear
//
// What reaching a level's door records in the save, split out of LevelScene so
// it can be unit-tested on the host (tests/host/test_level_records.cpp):
//   - the level's best time, if this clear is the first or a faster one,
//   - the no-death mark, if the attempt had no death (never removed again).
// Nothing is recorded while records are not allowed (Assist Mode, see
// LevelManager::records_allowed()) or for a level beyond the save's
// GameState::MAX_LEVELS. Returns true for a new best time ("New Best!").
// -----------------------------------------------------------------------------
inline auto record_clear(
    GameState& state,
    unsigned level_index,
    uint32_t frames,
    bool no_deaths,
    bool records_allowed) -> bool {
    if (!records_allowed || level_index >= unsigned(GameState::MAX_LEVELS)) {
        return false;
    }

    if (no_deaths) {
        set_no_death_clear(state, level_index);
    }

    uint32_t& best = state.best_time_frames[level_index];
    if (best != 0 && frames >= best) {
        return false;
    }
    best = frames;
    return true;
}
