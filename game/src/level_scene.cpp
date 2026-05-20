#include "level_scene.h"

extern bool game_finished;

LevelScene::LevelScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager)
    : _player(player),
      _levels(levels),
      _level_index(data_manager.load().level),
      _level_manager(&player, data_manager),
      _data_manager(data_manager),
      _transition_requested(false) {}

void LevelScene::init() {
    _level_manager.load(_levels[_level_index]);
}

void LevelScene::update() {
    if (!_transition_requested && _level_manager.update()) {
        _transition_requested = true;
        const unsigned int next_level_index = _level_index + 1u;
        if (next_level_index < static_cast<unsigned int>(_levels.size())) {
            // save on level transition
            auto& timer = _player.get_timer();
            auto& game_state = _data_manager.load();
            game_state.level = next_level_index;
            game_state.centis = timer.centis();
            game_state.seconds = timer.seconds();
            game_state.minutes = timer.minutes();
            _data_manager.save();

            core::SceneManager::instance().set_next_scene(
                bn::make_unique<LevelScene>(_player, _levels, _data_manager));
        } else {
            game_finished = true;
        }
    }
}
