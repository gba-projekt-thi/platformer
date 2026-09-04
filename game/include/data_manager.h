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
    // Loads save data from SRAM into runtime memory for the selected slot.
    void load_from_save();

    // Select active save slot index (0..2)
    void set_slot_index(int index);

    // Get currently selected slot index
    [[nodiscard]] auto slot_index() const -> int;

    // Mutable runtime state access.
    [[nodiscard]] auto state() -> GameState&;

    // Read-only runtime state access.
    [[nodiscard]] auto state() const -> const GameState&;

    // Saves current runtime state into SRAM.
    void save();

    // Resets both SRAM and runtime state.
    void reset();

   private:
    void _normalize_slot_index();
    void _load_slot_state();
    void _save_slot_state();
    void _clear_runtime_state();

    // Support 3 save slots by default.
    engine::save::SaveManager<GameState, 3> _save_mgr;

    // Currently selected save slot used for load/save/reset.
    int _slot_index{0};

    // Runtime copy of save data.
    GameState _game_state{};
};