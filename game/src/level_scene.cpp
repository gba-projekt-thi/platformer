#include "level_scene.h"

#include "bn_core.h"
#include "bn_unique_ptr.h"

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
}

void LevelScene::update() {
    // Prevent duplicate transitions.
    if (_transition_requested) {
        bn::core::update();
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

    const bool changes_world =
        !(_levels[_level_index].back_ground ==
          _levels[next_level_index].back_ground);

    auto next_scene = bn::make_unique<LevelScene>(
        _player, _levels, _data_manager);
    core::AudioTransitionOptions audio_options;
    audio_options.fade_music = changes_world;
    core::SceneManager::instance().set_next_scene(
        bn::move(next_scene), audio_options);
}
