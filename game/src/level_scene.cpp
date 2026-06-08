#include "level_scene.h"

extern bool game_finished;

LevelScene::LevelScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager)
    : _player(player),
      _levels(levels),

      // Runtime state access only.
      _level_index(data_manager.state().level),
      _level_manager(player, data_manager),
      _data_manager(data_manager),
      _transition_requested(false) {}

void LevelScene::init() {
    _level_manager.load(_levels[_level_index]);
    _player.set_visible(true);
    _player.set_hud_visible(true);
}

void LevelScene::update() {
    // Prevent duplicate transitions.
    if (_transition_requested) {
        return;
    }

    // Wait until level completion.
    if (!_level_manager.update()) {
        return;
    }
    _transition_requested = true;
    const unsigned int next_level_index = _level_index + 1u;

    // Game completed.
    if (next_level_index >= static_cast<unsigned int>(_levels.size())) {
        game_finished = true;
        return;
    }

    // Save progression state.
    auto& timer = _player.get_timer();
    auto& game_state = _data_manager.state();
    game_state.level = next_level_index;

    // IMPORTANT:
    // Persist deaths during level transitions.
    // Otherwise the next save can overwrite
    // newer runtime death values.
    game_state.deaths = _player.get_deaths();
    game_state.centis = timer.centis();
    game_state.seconds = timer.seconds();
    game_state.minutes = timer.minutes();
    _data_manager.save();

    // Load next level scene.
    _level_index = next_level_index;
    _transition_requested = false;

    this->init();
}