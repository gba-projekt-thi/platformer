#pragma once

#include <cstdint>

// Persistent save data.
//
// Layout rule: new fields are only ever APPENDED at the end, with a default
// member initializer, and need a SAVE_VERSION bump plus a line in
// game_state_migration.h - then existing saves keep working (the new field
// starts at its default). The save-migration unit tests lock the layout.
//
// Uses fixed-width integer types to ensure:
// - predictable memory layout
// - smaller save size
// - consistent ABI behavior on ARM
struct GameState {
    // Headroom above the current 25-level roster (see levels[] in
    // main.cpp / WorldIndex::WORLDS). 32 since schema 9 (24 before, see
    // game_state_migration::widen_level_records()): no_death_clears has one
    // bit per level, so this cannot grow further without a wider field.
    // Independent from Cfg::Level::Limits, which caps per-level entity
    // counts (platforms/triggers/traps), not level count.
    static constexpr int MAX_LEVELS = 32;

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

    // Bit i set = level i has been cleared at least once with zero
    // deaths during that clearing attempt (see SaveSyncController::
    // no_deaths_this_attempt()). Indexed the same as best_time_frames;
    // a single uint32_t covers exactly MAX_LEVELS=32 bits. Added in save
    // schema version 3 - older saves fail to
    // deserialize cleanly (same SERIALIZE_ERROR/fresh-GameState{}
    // fallback as furthest_level's v2 addition, see above).
    uint32_t no_death_clears = 0;

    // Personal achievement: set once the game has been completed (see
    // LevelScene::_finalize_completion()), never cleared afterward -
    // Hard Mode stays unlocked across future playthroughs on this slot.
    // Preserved across DataManager::reset() like best_time_frames/
    // no_death_clears above.
    bool hard_mode_unlocked = false;

    // Current-playthrough toggle (PauseController's Options sub-menu,
    // only togglable once hard_mode_unlocked is true). Scales trap
    // speed via LevelManager::trap_speed_multiplier() in
    // TrapFactory::create(). NOT preserved across DataManager::reset() -
    // resets to off like the audio volume settings, unlike the
    // achievement flag above. Both added in save schema version 4.
    bool hard_mode_enabled = false;

    // Current-playthrough toggle (PauseController's Options sub-menu),
    // available from the start - no unlock. Mirror image of Hard Mode:
    // halves Moving/Chase/Ambush trap speeds (LevelManager::
    // trap_speed_multiplier()) and widens coyote time / jump buffer
    // (Cfg::AssistMode). Mutually exclusive with hard_mode_enabled.
    // NOT preserved across DataManager::reset() - resets to off like
    // hard_mode_enabled and the audio volume settings. Added in save
    // schema version 5.
    bool assist_mode_enabled = false;
};

inline void set_no_death_clear(GameState& state, unsigned level_index) {
    if (level_index < 32) {
        state.no_death_clears |= (uint32_t(1) << level_index);
    }
}

[[nodiscard]] inline bool has_no_death_clear(
    const GameState& state,
    unsigned level_index) {
    return level_index < 32 &&
           (state.no_death_clears & (uint32_t(1) << level_index)) != 0;
}
