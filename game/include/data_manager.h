#pragma once

#include "game_state.h"
#include "save_manager.h"

// Handles save persistence and runtime game state ownership.
//
// Important:
// - load_from_save() performs actual SRAM loading.
// - state() returns the in-memory runtime state.
// - Avoid calling save/load repeatedly during gameplay.
class DataManager {
   public:
    // Loads save data from SRAM into runtime memory.
    void load_from_save();

    // Mutable runtime state access.
    [[nodiscard]] GameState& state();

    // Read-only runtime state access.
    [[nodiscard]] const GameState& state() const;

    // Saves current runtime state into SRAM.
    void save();

    // Resets both SRAM and runtime state.
    void reset();

   private:
    engine::save::SaveManager<GameState, 1> _save_mgr;

    // Runtime copy of save data.
    GameState _game_state{};
};