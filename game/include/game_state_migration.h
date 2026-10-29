#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game_state.h"
#include "save_header.h"

// -----------------------------------------------------------------------------
// GameState save migration
//
// A new GameState field does not wipe saves. SaveManager::load() takes
// game_state_valid_bytes() and, for a payload written by an older schema
// version, copies only the bytes that existed then; fields added later keep
// their default member initializers.
//
// Adding a field (the whole procedure):
//   1. APPEND it at the END of GameState, with a default member initializer.
//      Never insert, reorder or resize existing fields - every old layout
//      must stay a prefix of the new one.
//   2. Bump engine::save::SAVE_VERSION (save_header.h).
//   3. Add a line for the previous version below:
//          if (version < <new version>) return offsetof(GameState, <new
//          field>);
//      (offsetof, not sizeof: the field may fit into tail padding, so the
//      size can stay the same while the old padding byte is meaningless.)
//   4. Update the layout lock in tests/host/test_save_migration.cpp.
//
// Inserting a level into main.cpp's levels[] (anywhere but at the end) moves
// every level index behind it, and the save stores those indices (level,
// furthest_level, entry_level, best_time_frames, no_death_clears). Bump
// SAVE_VERSION and add an insert_level() line to upgrade() below; DataManager
// calls it with the version the slot was written with.
//
// Saves older than MIN_VERSION (schema 1-4) have layouts that are not a prefix
// of the current one (fields were inserted in the middle back then); they stay
// incompatible and fall back to a fresh state.
//
// Butano-free, so it is unit-tested on the host.
// -----------------------------------------------------------------------------

namespace game_state_migration {

inline constexpr uint16_t MIN_VERSION = 5;

// Schema 5-8 held 24 best times (120 bytes in all); schema 9 holds 32, so
// what came after them sits further back (see widen_level_records()).
inline constexpr int V8_LEVELS = 24;
inline constexpr int V8_SIZE = 120;

// Matches engine::save::ValidBytesFn.
inline auto valid_bytes(uint16_t version) -> int {
    if (version < MIN_VERSION) {
        return 0;
    }
    // The one exception to "append only": the whole old payload is read and
    // widen_level_records() moves its tail into place.
    if (version < 9) {
        return V8_SIZE;
    }
    if (version < 10) {
        return static_cast<int>(offsetof(GameState, entry_level));
    }
    if (version < 12) {
        return static_cast<int>(offsetof(GameState, play_frames));
    }
    // Add "if (version < N) return offsetof(GameState, field);" lines here,
    // newest first, when a field is appended (see the procedure above).
    return static_cast<int>(sizeof(GameState));
}

// Moves everything indexed by level from `index` on up by one; the new level
// at `index` has no record yet.
inline void insert_level(GameState& state, int index) {
    for (int i = GameState::MAX_LEVELS - 1; i > index; --i) {
        state.best_time_frames[i] = state.best_time_frames[i - 1];
    }
    state.best_time_frames[index] = 0;

    for (int i = GameState::MAX_LEVELS - 1; i > index; --i) {
        state.level_deaths[i] = state.level_deaths[i - 1];
    }
    state.level_deaths[index] = 0;

    const uint32_t below = (uint32_t(1) << index) - 1;
    state.no_death_clears = (state.no_death_clears & below) |
                            ((state.no_death_clears & ~below) << 1);

    if (state.level >= index) {
        ++state.level;
    }
    if (state.furthest_level >= index) {
        ++state.furthest_level;
    }
    if (state.entry_level >= index) {
        ++state.entry_level;
    }
}

// A schema 5-8 payload read into the schema 9 layout: its no_death_clears
// and mode flags landed in best_time_frames[24..25]. Moves them to their
// fields and clears the best times of the 8 new slots.
inline void widen_level_records(GameState& state) {
    uint8_t tail
        [V8_SIZE - offsetof(GameState, best_time_frames) -
         V8_LEVELS * sizeof(uint32_t)];
    std::memcpy(tail, &state.best_time_frames[V8_LEVELS], sizeof(tail));
    std::memcpy(&state.no_death_clears, tail, sizeof(uint32_t));
    state.hard_mode_unlocked = tail[4] != 0;
    state.hard_mode_enabled = tail[5] != 0;
    state.assist_mode_enabled = tail[6] != 0;
    for (int i = V8_LEVELS; i < GameState::MAX_LEVELS; ++i) {
        state.best_time_frames[i] = 0;
    }
}

// The levels inserted into the roster since `version`, each at its index in
// the roster of its time.
inline void insert_levels_since(GameState& state, uint16_t version) {
    // Oldest first, each at its index in the roster of its time.
    // Schema 6: LEVEL_WORLD5_CLIMB before World 5's boss.
    if (version < 6) {
        insert_level(state, 17);
    }
    // Schema 7: LEVEL_WORLD4_POGO as World 4's first level.
    if (version < 7) {
        insert_level(state, 10);
    }
    // Schema 8: LEVEL_WORLD3_TRICK before World 3's boss.
    if (version < 8) {
        insert_level(state, 9);
    }
    // Schema 9: a level before the bosses of World 1, 2 and 5.
    if (version < 9) {
        insert_level(state, 3);
        insert_level(state, 7);
        insert_level(state, 22);
    }
    // Schema 11: LEVEL_WORLD4_TRICK before World 4's boss.
    if (version < 11) {
        insert_level(state, 16);
    }
}

// Brings a slot written with schema `version` up to the current one.
inline void upgrade(GameState& state, uint16_t version) {
    if (version < 9) {
        widen_level_records(state);
    }
    insert_levels_since(state, version);
}

}  // namespace game_state_migration
