#include "data_manager.h"

void DataManager::load_from_save() {
    if (_save_mgr.is_slot_used(0)) {
        _save_mgr.load(0, _game_state);
    }
}

GameState& DataManager::state() {
    return _game_state;
}

const GameState& DataManager::state() const {
    return _game_state;
}

void DataManager::save() {
    _save_mgr.save(0, _game_state);
}

void DataManager::reset() {
    // Reset runtime state first.
    _game_state = {};

    // Then persist the cleared save.
    _save_mgr.save(0, _game_state);
}