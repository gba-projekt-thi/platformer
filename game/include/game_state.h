#pragma once

#include <cstdint>

// Persistent save data.
//
// Uses fixed-width integer types to ensure:
// - predictable memory layout
// - smaller save size
// - consistent ABI behavior on ARM
struct GameState {
    // Headroom above the current 9-level roster (see levels[] in
    // main.cpp / WorldIndex::WORLDS). Bump if the game ever grows past
    // this many levels. Independent from Cfg::Level::Limits, which caps
    // per-level entity counts (platforms/triggers/traps), not level
    // count.
    static constexpr int MAX_LEVELS = 16;

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

    // Per-save-slot sound settings, 0..4 (4 = full volume). Must match
    // AudioSettings::MAX_LEVEL - restored into that singleton via
    // LevelManager::restoreHUD().
    uint8_t music_volume = 4;
    uint8_t sfx_volume = 4;

    // Per-level personal-best clear time, in frames (60 = 1 second),
    // indexed by absolute level index (same indexing as the levels[]
    // array in main.cpp and WorldIndex::WORLDS' start_index/level_count).
    // 0 means "no record yet". Measured from level load to door reached
    // and deliberately NOT reset on death within an attempt - see
    // LevelManager::update()'s comment on _level_frame_count. Displayed
    // via timer.h's frames_to_time().
    uint32_t best_time_frames[MAX_LEVELS] = {};
};