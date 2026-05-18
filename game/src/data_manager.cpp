#include "data_manager.h"

GameState& DataManager::load() {
    if (save_mgr.is_slot_used(0)) {
        save_mgr.load(0, game_state);
    }
    return game_state;
}

void DataManager::save() {
    save_mgr.save(0, game_state);
}
