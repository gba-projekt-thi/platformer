#pragma once

#include "data_manager.h"
#include "player.h"
#include "timer.h"

// -----------------------------------------------------------------------------
// SaveSyncController
//
// Owns the runtime persistence policy for a level: watches the player's
// death counter, writes deaths/timer into the save state, and commits to
// SRAM. Extracted out of LevelManager so gameplay systems don't need to
// know about SRAM write timing/cost directly.
//
// NOTE:
// SRAM writes are relatively expensive. Consider batching saves later
// (see project backlog).
// -----------------------------------------------------------------------------
class SaveSyncController {
   public:
    SaveSyncController(Player& player, DataManager& data_manager);

    // Call once when a level finishes loading. Establishes the death-count
    // baseline used by sync() to detect new deaths during this level.
    void reset_baseline();

    // Call once per frame. Persists deaths/timer to the runtime save state
    // and writes SRAM if the player's death count changed since the last
    // call. Returns true exactly when a save was triggered by a new death,
    // so callers can react (e.g. reset traps).
    bool sync();

    // Unconditionally persists deaths/timer and writes SRAM, without
    // requiring a death to have occurred. Used by callers that need to
    // guarantee progress is saved (e.g. leaving to the title screen from
    // the pause menu).
    void force_save();

   private:
    void _save_progress();

    Player& _player;
    DataManager& _data_manager;
    unsigned _last_death_ct = 0;
};