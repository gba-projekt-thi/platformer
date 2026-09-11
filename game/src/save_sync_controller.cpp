#include "save_sync_controller.h"

SaveSyncController::SaveSyncController(
    Player& player,
    DataManager& data_manager)
    : _player(player), _data_manager(data_manager) {}

void SaveSyncController::reset_baseline() {
    _last_death_ct = _player.get_deaths();
}

bool SaveSyncController::sync() {
    if (_last_death_ct == _player.get_deaths()) {
        return false;
    }

    _last_death_ct = _player.get_deaths();
    _save_progress();
    return true;
}

void SaveSyncController::force_save() {
    _last_death_ct = _player.get_deaths();
    _save_progress();
}

void SaveSyncController::_save_progress() {
    // Runtime state access only. Deliberately does not touch
    // game_state.level - callers that also advance to a new level update
    // that separately (see LevelScene::update()).
    auto& game_state = _data_manager.state();
    game_state.deaths = _player.get_deaths();

    Timer& timer = _player.get_timer();
    game_state.centis = timer.centis();
    game_state.seconds = timer.seconds();
    game_state.minutes = timer.minutes();

    _data_manager.save();
}