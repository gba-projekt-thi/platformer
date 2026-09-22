#include "kissing_scene.h"

#include "audio_settings.h"
#include "bn_core.h"
#include "bn_music_items.h"
#include "bn_regular_bg_items_kissingscene.h"
#include "bn_sound_items.h"

#include "cfg.h"
#include "data_manager.h"
#include "level_manager.h"
#include "player.h"
#include "summary_scene.h"

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

    // Show the run summary (total time, deaths, no-death clears) before
    // returning to the title. DataManager::reset() moved to SummaryScene
    // - it now runs once the player dismisses that screen, not here, so
    // the numbers it reads are still this run's live state.
    auto summary_scene = bn::make_unique<SummaryScene>(
        _player, _levels, _data_manager, _level_manager);
    core::SceneManager::instance().set_next_scene(bn::move(summary_scene));

    // Play a confirmation sound when the transition begins.
    AudioSettings::instance().play_sfx(bn::sound_items::confirm);
    bn::core::update();
}