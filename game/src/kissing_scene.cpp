#include "kissing_scene.h"

#include "bn_core.h"
#include "bn_music_items.h"
#include "bn_regular_bg_items_kissingscene.h"

KissingScene::KissingScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager),
      _timer(Cfg::Sleep::KISSING_SCENE),
      _transition_requested(false) {}

void KissingScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);

    _bg.emplace(bn::regular_bg_items::kissingscene.create_bg(0, 0));
    _bg->set_priority(3);
}

void KissingScene::update() {
    if (_transition_requested) {
        bn::core::update();
        return;
    }

    if (--_timer > 0) {
        bn::core::update();
        return;
    }

    _transition_requested = true;

    _data_manager.reset();
    auto restart_scene = bn::make_unique<StartScene>(
        _player, _levels, _data_manager, _level_manager);
    core::SceneManager::instance().set_next_scene(bn::move(restart_scene));
    bn::sound_items::confirm.play();
    bn::core::update();
}
