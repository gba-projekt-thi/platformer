#pragma once

#include <cstddef>
#include <cstdint>

#include "game_state.h"
#include "save_header.h"

// -----------------------------------------------------------------------------
// GameState save migration
//
// A new GameState field no longer wipes every save. SaveManager::load() takes
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
// furthest_level, best_time_frames, no_death_clears). Bump SAVE_VERSION and
// add an insert_level() line to upgrade() below; DataManager calls it with
// the version the slot was written with.
//
// Saves older than MIN_VERSION (schema 1-4) have layouts that are not a prefix
// of the current one (fields were inserted in the middle back then); they stay
// incompatible and fall back to a fresh state, as before.
//
// Butano-free, so it is unit-tested on the host.
// -----------------------------------------------------------------------------

namespace game_state_migration {

inline constexpr uint16_t MIN_VERSION = 5;

// Matches engine::save::ValidBytesFn.
inline auto valid_bytes(uint16_t version) -> int {
    if (version < MIN_VERSION) {
        return 0;
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

    const uint32_t below = (uint32_t(1) << index) - 1;
    state.no_death_clears = (state.no_death_clears & below) |
                            ((state.no_death_clears & ~below) << 1);

    if (state.level >= index) {
        ++state.level;
    }
    if (state.furthest_level >= index) {
        ++state.furthest_level;
    }
}

// Applies the roster changes made since `version` (the slot's schema).
inline void upgrade(GameState& state, uint16_t version) {
    // Schema 6: LEVEL_WORLD5_CLIMB before World 5's boss.
    if (version < 6) {
        insert_level(state, 17);
    }
}

}  // namespace game_state_migration
