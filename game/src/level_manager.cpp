#include "level_manager.h"
#include "trap_factory.h"

LevelManager::LevelManager(Player& player, DataManager& data_manager)
    : _player(player), _pause_controller(), _data_manager(data_manager) {
    restoreHUD();
}

void LevelManager::restoreHUD() {
    // -------------------------------------------------------------------------
    // Restore persistent runtime state
    // -------------------------------------------------------------------------

    Timer& timer = _player.get_timer();

    // Access already-loaded runtime state.
    auto& game_state = _data_manager.state();
    _player.set_deaths(game_state.deaths);
    timer.set_time(game_state.centis, game_state.seconds, game_state.minutes);
}

auto LevelManager::get_trigger(int trigger_index) -> Trigger& {
    const int trigger_count = _triggers.size();
    if (trigger_index >= 0 && trigger_index < trigger_count) {
        return _triggers[trigger_index];
    }
    BN_LOG(
        "[ERROR] level_manager: "
        "invalid trigger index, fallback to trigger 0");
    return _triggers[0];
}

void LevelManager::_reset_traps() {
    for (auto& trap : _traps) {
        trap->reset();
    }
}

void LevelManager::_load_player_spawn(const LevelData& level) {
    _player.teleport_to(level.player_data.x, level.player_data.y);
    _player.set_spawn_point(level.player_data.x, level.player_data.y);
    _last_death_ct = _player.get_deaths();
}

void LevelManager::_load_door(const LevelData& level) {
    _door.emplace(level.door.x, level.door.y);
}

void LevelManager::_load_music(const LevelData& level) {
    if (!_music.has_value() || *_music != level.music) {
        _music.emplace(level.music);
        bn::music::play(*_music);
    }
}

void LevelManager::_load_background(const LevelData& level) {
    _background.reset();
    _background.emplace(level.back_ground.create_bg(0, 0));
    _background->set_priority(3);
    _background->set_blending_enabled(true);
}

void LevelManager::unload() {
    // Release all per-level VRAM/SRAM-adjacent resources: door, background,
    // platforms, platform bodies, triggers and traps. Their destructors
    // automatically unregister from CollisionRegistry / SpriteRegistry, so
    // clearing/resetting the owning containers is sufficient.
    //
    // This is called every time a LevelScene is destroyed (both on
    // level-to-level transitions and when the game finishes), which is why
    // it must run BEFORE the next scene allocates its own background -
    // otherwise the old level's BG palette stays resident and can exhaust
    // the BPP8 palette budget when the endgame background loads.
    //
    // NOTE: `_music` is intentionally left untouched here. `_load_music()`
    // compares the incoming level's track against `_music` to decide
    // whether to restart playback; resetting it here would force an
    // audible restart even when two consecutive levels share the same
    // track.
    _door.reset();
    _background.reset();
    _clear_runtime_state();
}

void LevelManager::_clear_runtime_state() {
    _platforms.clear();
    _platform_bodies.clear();
    _triggers.clear();
    _traps.clear();
}

void LevelManager::_validate_level(const LevelData& level) {
    BN_ASSERT(
        unsigned(level.platform_count) < Cfg::Level::Limits::PLATFORMS,
        "Too many platforms");
    BN_ASSERT(
        unsigned(level.trigger_count) < Cfg::Level::Limits::TRIGGERS,
        "Too many triggers");
    BN_ASSERT(
        unsigned(level.trap_count) < Cfg::Level::Limits::TOTAL_TRAPS,
        "Too many traps");
}

void LevelManager::_load_platforms(const LevelData& level) {
    for (int i = 0; i < level.platform_count; ++i) {
        const PlatformData& platform = level.platforms[i];
        const int graphics_count =
            platform.sprite.tiles_item().graphics_count();
        bn::sprite_ptr sprite =
            platform.sprite.create_sprite(platform.x, platform.y);
        sprite.set_tiles(platform.sprite.tiles_item().create_tiles(
            platform.sprite_index % graphics_count));
        sprite.set_blending_enabled(true);
        _platforms.push_back(bn::move(sprite));
        _platform_bodies.emplace_back(
            platform.x, platform.y, _platforms.back().dimensions().width(),
            _platforms.back().dimensions().height(), Cfg::Layer::PLATFORM);
    }
}

void LevelManager::_load_triggers(const LevelData& level) {
    for (int i = 0; i < level.trigger_count; ++i) {
        const TriggerData& trigger = level.triggers[i];
        _triggers.emplace_back(
            trigger.x, trigger.y, trigger.width, trigger.height);
    }

    if (_triggers.empty()) {
        _triggers.emplace_back(1000, 1000, 0, 0, true);
    }
}

void LevelManager::_load_traps(const LevelData& level) {
    for (int i = 0; i < level.trap_count; ++i) {
        _traps.push_back(TrapFactory::create(level.traps[i], *this));
    }
}

void LevelManager::load(const LevelData& level) {
    _pause_controller.reset();
    _load_player_spawn(level);
    _load_door(level);
    _load_music(level);
    _load_background(level);
    _clear_runtime_state();
    _validate_level(level);
    _load_platforms(level);
    _load_triggers(level);
    _load_traps(level);
}

auto LevelManager::update() -> bool {
    // -------------------------------------------------------------------------
    // Pause
    // -------------------------------------------------------------------------

    const auto pause_action = _pause_controller.update();
    if (pause_action == PauseController::Action::DeathRequested) {
        _player.death();
    }
    if (_pause_controller.paused()) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Physics
    // -------------------------------------------------------------------------

    CollisionRegistry::instance().update_all();

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------

    SpriteRegistry::instance().sync_all(Camera::instance());
    bn::core::update();

    // -------------------------------------------------------------------------
    // Door Completion
    // -------------------------------------------------------------------------

    if (_door && _door->reached()) {
        return true;
    }

    // -------------------------------------------------------------------------
    // Death Synchronization
    // -------------------------------------------------------------------------

    if (_last_death_ct != _player.get_deaths()) {
        _last_death_ct = _player.get_deaths();

        // Runtime state access only.
        auto& game_state = _data_manager.state();
        game_state.deaths = _last_death_ct;
        Timer& timer = _player.get_timer();
        game_state.centis = timer.centis();
        game_state.seconds = timer.seconds();
        game_state.minutes = timer.minutes();

        // NOTE:
        // SRAM writes are relatively expensive.
        // Consider batching saves later.
        _data_manager.save();
        _reset_traps();
    }
    return false;
}