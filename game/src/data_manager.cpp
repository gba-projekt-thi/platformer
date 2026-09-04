#include "data_manager.h"

void DataManager::_normalize_slot_index() {
    if (_slot_index < 0) {
        _slot_index = 0;
    }
    if (_slot_index >= _save_mgr.slot_count()) {
        _slot_index = _save_mgr.slot_count() - 1;
    }
}

void DataManager::_load_slot_state() {
    if (_save_mgr.is_slot_used(_slot_index)) {
        _save_mgr.load(_slot_index, _game_state);
    }
}

void DataManager::_save_slot_state() {
    _save_mgr.save(_slot_index, _game_state);
}

void DataManager::_clear_runtime_state() {
    _game_state = {};
}

void DataManager::load_from_save() {
    _load_slot_state();
}

auto DataManager::state() -> GameState& {
    return _game_state;
}

auto DataManager::state() const -> const GameState& {
    return _game_state;
}

void DataManager::save() {
    _save_slot_state();
}

void DataManager::reset() {
    _clear_runtime_state();
    _save_slot_state();
}

void DataManager::set_slot_index(int index) {
    _slot_index = index;
    _normalize_slot_index();
}

auto DataManager::slot_index() const -> int {
    return _slot_index;
}