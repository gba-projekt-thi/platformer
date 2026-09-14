#include "data_manager.h"

void DataManager::load_from_save() {
    if (_save_mgr.is_slot_used(_slot_index)) {
        if (_save_mgr.load(_slot_index, _game_state) !=
            engine::save::SaveResult::OK) {
            // Incompatible/corrupt slot (e.g. a save written before
            // furthest_level existed) - fall back to a clean state
            // instead of silently keeping whatever was already in
            // memory.
            _game_state = {};
        }
    }

    // Defensive: furthest_level must never trail the resume point.
    // Guards against any legacy or otherwise inconsistent save data.
    if (_game_state.furthest_level < _game_state.level) {
        _game_state.furthest_level = _game_state.level;
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
    // Preserve per-level best times across a full-game reset. They are
    // personal records, not "current run" progress (level/deaths/timer),
    // which is what this reset is meant to clear on finishing the game.
    uint32_t best_times[GameState::MAX_LEVELS];
    for (int i = 0; i < GameState::MAX_LEVELS; ++i) {
        best_times[i] = _game_state.best_time_frames[i];
    }

    // Reset runtime state.
    _game_state = {};

    for (int i = 0; i < GameState::MAX_LEVELS; ++i) {
        _game_state.best_time_frames[i] = best_times[i];
    }

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