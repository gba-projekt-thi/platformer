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
    // Hide the player and HUD while the endgame splash is visible.
    _player.set_visible(false);
    _player.set_hud_visible(false);

    // Create the kiss background once and keep it alive for the duration of
    // the scene. The background releases automatically when the scene exits.
    _bg.emplace(bn::regular_bg_items::kissingscene.create_bg(0, 0));
    _bg->set_priority(3);
}

void KissingScene::update() {
    if (_transition_requested) {
        // Keep updating the engine until the next scene transition finishes.
        bn::core::update();
        return;
    }

    if (--_timer > 0) {
        // Continue rendering the kiss scene until the timer expires.
        bn::core::update();
        return;
    }

    _transition_requested = true;

    // Reset game save state after full completion.
    _data_manager.reset();

    // Return to the start scene as the next scene.
    auto restart_scene = bn::make_unique<StartScene>(
        _player, _levels, _data_manager, _level_manager);
    core::SceneManager::instance().set_next_scene(bn::move(restart_scene));

    // Play a confirmation sound when the transition begins.
    bn::sound_items::confirm.play();
    bn::core::update();
}
