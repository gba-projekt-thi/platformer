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

}  // namespace game_state_migration
