#pragma once

#include <cstdint>

// Persistent save data.
//
// Uses fixed-width integer types to ensure:
// - predictable memory layout
// - smaller save size
// - consistent ABI behavior on ARM
struct GameState {
    int16_t level = 0;
    uint16_t deaths = 0;  // unsigned: matches Player::set_deaths(unsigned int)
                          // and LevelManager's unsigned _last_death_ct — avoids
                          // silent narrowing to negative values.

    uint16_t centis = 0;
    uint16_t seconds = 0;
    uint16_t minutes = 0;

    // Furthest level index ever reached through normal progression (door
    // reached; only ever increases). Decoupled from `level` (the resume
    // point) so that jumping back to replay an earlier level via
    // LevelSelectScene does not re-lock levels beyond it. Added in save
    // schema version 2 (see save_header.h::SAVE_VERSION) - older saves
    // fail to deserialize cleanly (SaveManager::load() returns
    // SERIALIZE_ERROR due to the size mismatch) and DataManager falls
    // back to a fresh GameState{} rather than reading garbage.
    int16_t furthest_level = 0;
};