#include "data_manager.h"

void DataManager::load_from_save() {
    if (_save_mgr.is_slot_used(_slot_index)) {
        _save_mgr.load(_slot_index, _game_state);
    }
}

GameState& DataManager::state() {
    return _game_state;
}

const GameState& DataManager::state() const {
    return _game_state;
}

void DataManager::save() {
    _save_mgr.save(_slot_index, _game_state);
}

void DataManager::reset() {
    // Reset runtime state first.
    _game_state = {};

    // Then persist the cleared save.
    _save_mgr.save(_slot_index, _game_state);
}

void DataManager::set_slot_index(int index) {
    if (index < 0)
        index = 0;
    if (index >= _save_mgr.slot_count())
        index = _save_mgr.slot_count() - 1;
    _slot_index = index;
}

int DataManager::slot_index() const {
    return _slot_index;
}