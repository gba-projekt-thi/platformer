#pragma once

#include "save_manager.h"

#include "game_state.h"

class DataManager {
   public:
    GameState& load();
    void save();

   private:
    engine::save::SaveManager<GameState, 1> save_mgr;
    GameState game_state{};
};