#include "level_scene.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_unique_ptr.h"

#include "data_manager.h"
#include "game_session.h"
#include "level_manager.h"
#include "player.h"
#include "start_scene.h"

LevelScene::LevelScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _player(player),
      _levels(levels),

      // Runtime state access only.
      _level_index(data_manager.state().level),
      _level_manager(level_manager),
      _data_manager(data_manager),
      _transition_requested(false) {}

LevelScene::~LevelScene() {
    // Ensure all level-specific resources are released when the scene ends.
    // This prevents BPP8 palette exhaustion when loading the endgame
    // background.
    _level_manager.unload();
}

void LevelScene::init() {
    _level_manager.load(_levels[_level_index]);
    _player.set_visible(true);
    _player.set_hud_visible(true);
}

void LevelScene::update() {
    // Waiting for the player to acknowledge a new best-time banner before
    // actually committing the level transition (see _new_best_pending's
    // comment in level_scene.h).
    if (_new_best_pending) {
        if (bn::keypad::a_pressed() || bn::keypad::start_pressed()) {
            _hide_new_best_banner();
            _new_best_pending = false;

            if (_next_is_game_completion) {
                _finalize_completion();
            } else {
                _finalize_advance(_pending_next_level_index);
            }
        }
        bn::core::update();
        return;
    }

    // Prevent duplicate transitions.
    if (_transition_requested) {
        bn::core::update();
        return;
    }

    const LevelManager::UpdateResult result = _level_manager.update();
    if (result == LevelManager::UpdateResult::None) {
        return;
    }

    // -------------------------------------------------------------------------
    // Pause menu: back to the title screen.
    // -------------------------------------------------------------------------

    if (result == LevelManager::UpdateResult::ReturnToTitle) {
        _transition_requested = true;
        auto next_scene = bn::make_unique<StartScene>(
            _player, _levels, _data_manager, _level_manager);
        core::SceneManager::instance().set_next_scene(bn::move(next_scene));
        return;
    }

    // -------------------------------------------------------------------------
    // Door reached: advance to the next level (or finish the game).
    // -------------------------------------------------------------------------

    // Record a new best-clear-time for the level just finished, before
    // anything else touches game_state - covers both the "advance to
    // next level" and "game completed" paths below, since both start
    // from having reached this level's door.
    bool new_best = false;
    if (_level_index < unsigned(GameState::MAX_LEVELS)) {
        auto& game_state = _data_manager.state();
        const unsigned frames = _level_manager.level_frame_count();
        uint32_t& best = game_state.best_time_frames[_level_index];
        if (best == 0 || frames < best) {
            best = frames;
            new_best = true;
        }
    }

    const unsigned int next_level_index = _level_index + 1u;
    const bool completes_game =
        next_level_index >= static_cast<unsigned int>(_levels.size());

    if (new_best) {
        // Hold here - _finalize_advance()/_finalize_completion() run once
        // the player acknowledges the banner (see top of this function).
        _next_is_game_completion = completes_game;
        _pending_next_level_index = next_level_index;
        _new_best_pending = true;
        _show_new_best_banner();
        return;
    };
    if (completes_game) {
        _finalize_completion();
    } else {
        _finalize_advance(next_level_index);
    }
}

void LevelScene::_finalize_completion() {
    _transition_requested = true;

    // Persist the best time recorded in update(). Unlike
    // _finalize_advance(), nothing else on this path calls save() -
    // KissingScene later calls DataManager::reset(), which preserves
    // best_time_frames across the reset, but this level's newly-set
    // record still needs to reach SRAM first.
    _data_manager.save();
    GameSession::instance().set_finished();
}

void LevelScene::_finalize_advance(unsigned int next_level_index) {
    _transition_requested = true;

    auto& timer = _player.get_timer();
    auto& game_state = _data_manager.state();
    game_state.level = next_level_index;
    if (next_level_index >
        static_cast<unsigned int>(game_state.furthest_level)) {
        game_state.furthest_level = static_cast<int16_t>(next_level_index);
    }

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
        _player, _levels, _data_manager, _level_manager);
    core::AudioTransitionOptions audio_options;
    audio_options.fade_music = changes_world;
    core::SceneManager::instance().set_next_scene(
        bn::move(next_scene), audio_options);
}

void LevelScene::_show_new_best_banner() {
    _banner_text_gen.emplace(common::variable_8x16_sprite_font);
    _banner_text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _banner_text_gen->set_blending_enabled(true);
    _banner_text_gen->generate(
        Cfg::NewBestBanner::X, Cfg::NewBestBanner::Y, "New Best!",
        _banner_sprites);

    for (bn::sprite_ptr& sprite : _banner_sprites) {
        sprite.set_blending_enabled(true);
    }
}

void LevelScene::_hide_new_best_banner() {
    for (bn::sprite_ptr& sprite : _banner_sprites) {
        sprite.set_visible(false);
    }
    _banner_sprites.clear();
    _banner_text_gen.reset();
}