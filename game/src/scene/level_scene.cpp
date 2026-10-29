#include "level_scene.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "audio_settings.h"
#include "cfg.h"
#include "data_manager.h"
#include "game_session.h"
#include "level_manager.h"
#include "level_records.h"
#include "player.h"
#include "replay_capture.h"
#include "start_scene.h"
#include "story.h"
#include "story_scene.h"
#include "text_buffer.h"
#include "timer.h"
#include "tour.h"

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
    replay_capture::level_started(_player, _level_index);
    _level_manager.load(_levels[_level_index]);
    tour::level_started(_player, _level_index);
    _player.set_visible(true);
    _player.set_hud_visible(true);
    _show_hint_if_needed();
}

void LevelScene::_show_hint_if_needed() {
    const char* text = _levels[_level_index].intro_hint;
    const uint32_t best =
        _level_index < unsigned(GameState::MAX_LEVELS)
            ? _data_manager.state().best_time_frames[_level_index]
            : 0;
    _hint_text = hint::text_to_show(
        text, best, _data_manager.state().furthest_level, _level_index);
    if (_hint_text == nullptr) {
        return;
    }

    // A boss level's hint is about the arena: it starts there (_update_hint).
    if (_hint.show(_level_manager.has_boss())) {
        _draw_hint();
    }
}

void LevelScene::_draw_hint() {
    _hint_text_gen.emplace(common::variable_8x16_sprite_font);
    _hint_text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _hint_text_gen->set_blending_enabled(true);
    _hint_text_gen->set_center_alignment();
    _hint_text_gen->generate(0, Cfg::Hint::Y, _hint_text, _hint_sprites);
    for (bn::sprite_ptr& sprite : _hint_sprites) {
        sprite.set_blending_enabled(true);
    }
}

void LevelScene::_update_hint() {
    if (_hint_text == nullptr) {
        return;
    }
    switch (_hint.update(
        _level_manager.paused(), _level_manager.boss_fighting(),
        bn::keypad::b_pressed(), !_hint_sprites.empty())) {
        case hint::Sprites::Draw:
            _draw_hint();
            break;
        case hint::Sprites::Release:
            _hint_sprites.clear();
            break;
        case hint::Sprites::Finish:
            _hint_sprites.clear();
            _hint_text_gen.reset();
            _hint_text = nullptr;
            break;
        case hint::Sprites::Keep:
        default:
            break;
    }
}

void LevelScene::update() {
    _update_hint();

    // The clear moment: only the picture's blink and the sound go on.
    if (_clear_moment.running()) {
        _clear_moment.tick();
        _level_manager.flash_door(_clear_moment.lit());
        if (!_clear_moment.running()) {
            _end_clear_moment();
        }
        bn::core::update();
        return;
    }

    // Waiting for the player to acknowledge a new best-time banner before
    // actually committing the level transition (see _new_best_pending's
    // comment in level_scene.h).
    if (_new_best_pending) {
        if (bn::keypad::a_pressed() || bn::keypad::start_pressed()) {
            _hide_new_best_banner();
            _new_best_pending = false;
            _leave_level();
        }
        bn::core::update();
        return;
    }

    // Prevent duplicate transitions.
    if (_transition_requested) {
        bn::core::update();
        return;
    }

    // The hint's sprites are freed before the pause overlay allocates its own
    // (hint::step_level); they are drawn again once the game runs.
    struct Step {
        LevelScene& scene;
        static bool start_pressed() { return bn::keypad::start_pressed(); }
        void release_hint_sprites() { scene._hint_sprites.clear(); }
        auto update_level() { return scene._level_manager.update(); }
    } step{*this};
    const LevelManager::UpdateResult result = hint::step_level(step);
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

    replay_capture::level_finished();

    // Record the clear (best time, no-death mark) before anything else
    // touches game_state - covers both the "advance to next level" and "game
    // completed" paths below. Nothing is recorded in Assist Mode; the level
    // still counts as cleared for progress and unlocks below.
    const bool new_best = record_clear(
        _data_manager.state(), _level_index, _level_manager.level_frame_count(),
        _level_manager.no_deaths_this_attempt(),
        _level_manager.records_allowed());

    const unsigned int next_level_index = _level_index + 1u;
    const bool completes_game =
        next_level_index >= static_cast<unsigned int>(_levels.size());

    // Hold here - _finalize_advance()/_finalize_completion() run once the
    // clear moment is over and, for a new best, the player acknowledged the
    // banner (see top of this function).
    _next_is_game_completion = completes_game;
    _pending_next_level_index = next_level_index;
    _clear_new_best = new_best;
    _start_clear_moment();
}

void LevelScene::_start_clear_moment() {
    // The level is done: the hint has nothing left to say.
    _hint_sprites.clear();
    _hint_text = nullptr;
    _hint_text_gen.reset();

    AudioSettings::instance().play_sfx(bn::sound_items::confirm);
    _banner_text_gen.emplace(common::variable_8x16_sprite_font);
    _banner_text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _banner_text_gen->set_center_alignment();
    _banner_text_gen->generate(
        0, Cfg::ClearMoment::Y, "Clear!", _clear_sprites);
    _clear_moment.start();
    _level_manager.flash_door(true);
    bn::core::update();
}

void LevelScene::_end_clear_moment() {
    _level_manager.flash_door(false);
    _clear_sprites.clear();
    _banner_text_gen.reset();
    if (_clear_new_best) {
        _new_best_pending = true;
        _show_new_best_banner();
        return;
    }
    _leave_level();
}

void LevelScene::_leave_level() {
    if (_next_is_game_completion) {
        _finalize_completion();
    } else {
        _finalize_advance(_pending_next_level_index);
    }
}

void LevelScene::_finalize_completion() {
    _transition_requested = true;
    _data_manager.state().hard_mode_unlocked = true;
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
    // The first arrival in a world tells its part of the story.
    const story::Page* page = story::page_on_advance(
        game_state.furthest_level, int(next_level_index));
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

    core::AudioTransitionOptions audio_options;
    audio_options.fade_music = changes_world;
    if (page != nullptr) {
        auto next_scene = bn::make_unique<StoryScene>(
            *page, _player, _levels, _data_manager, _level_manager);
        core::SceneManager::instance().set_next_scene(
            bn::move(next_scene), audio_options);
        return;
    }
    auto next_scene = bn::make_unique<LevelScene>(
        _player, _levels, _data_manager, _level_manager);
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

    // Level-specific best time, e.g. "01:23.45" (same format as
    // LevelSelectScene).
    if (_level_index < unsigned(GameState::MAX_LEVELS)) {
        TextBuffer<9> time;
        time.append_time(frames_to_time(
            _data_manager.state().best_time_frames[_level_index]));
        _banner_text_gen->generate(
            Cfg::NewBestBanner::X, Cfg::NewBestBanner::Y + 16, time.c_str(),
            _banner_sprites);
    }

    _banner_text_gen->generate(
        Cfg::NewBestBanner::X, Cfg::NewBestBanner::Y + 32,
        "Press A to continue", _banner_sprites);

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