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

Trigger& LevelManager::get_trigger(int trigger_index) {
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

void LevelManager::_save_progress() {
    // Runtime state access only. Deliberately does not touch
    // game_state.level - callers that also advance to a new level update
    // that separately (see LevelScene::update()).
    auto& game_state = _data_manager.state();
    game_state.deaths = _player.get_deaths();

    Timer& timer = _player.get_timer();
    game_state.centis = timer.centis();
    game_state.seconds = timer.seconds();
    game_state.minutes = timer.minutes();

    // NOTE:
    // SRAM writes are relatively expensive.
    // Consider batching saves later.
    _data_manager.save();
}

// -----------------------------------------------------------------------------
// load() stages
// -----------------------------------------------------------------------------

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
    // -------------------------------------------------------------------------
    // Background
    //
    // Always created at its authored position (0,0) and NEVER repositioned
    // afterwards - neither here nor in update(). The current background art
    // (level1/level2/level3/world4/world5) was authored as a single static
    // 240x160 screen, not a wide seamlessly-tileable world backdrop;
    // repositioning it with the camera wraps it at the GBA tilemap block
    // boundary (visible seam) and offsets it away from screen center in
    // wide levels. Until dedicated wide/tileable background art exists per
    // scrolling level, the background is a static backdrop while the
    // foreground (player/platforms/traps/door) scrolls normally via
    // SpriteRegistry.
    // -------------------------------------------------------------------------

    _background.reset();

    // Force a full engine frame between releasing the previous background's
    // tiles/palette and allocating the new one's. Without this, a normal
    // level-to-level scene transition destroys the old background and
    // creates the new one back-to-back within the same SceneManager::update()
    // call (no bn::core::update() runs between LevelManager::unload() and
    // load() during that transition), so the new background's VRAM write can
    // land in the same uncommitted batch as the old one's pending release -
    // observed as a level's background using stale palette colors from a
    // previously loaded level's background (but only when reached via normal
    // progression, not when loaded fresh from a save, where no prior
    // background was ever allocated).
    bn::core::update();

    _background.emplace(level.back_ground.create_bg(0, 0));
    _background->set_priority(3);
    _background->set_blending_enabled(true);
}

void LevelManager::_setup_camera(const LevelData& level) {
    Camera::instance().init(level.world_width, level.world_height);
    Camera::instance().follow(_player.pos.x, _player.pos.y);

    // Bounce boundary must match this level's world width, not the fixed
    // 240px screen default.
    _player.set_horizontal_bound(
        level.world_width / 2 - Cfg::Screen::EDGE_MARGIN);
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
            platform.x, platform.y, platform.width, platform.height,
            Cfg::Layer::PLATFORM);
        _platform_bodies.back().pos.offset_x = platform.offset_x;
        _platform_bodies.back().pos.offset_y = platform.offset_y;
    }
}

void LevelManager::_load_triggers(const LevelData& level) {
    for (int i = 0; i < level.trigger_count; ++i) {
        const TriggerData& trigger = level.triggers[i];
        _triggers.emplace_back(
            trigger.x, trigger.y, trigger.width, trigger.height);
    }

    // Fallback trigger prevents invalid references.
    if (_triggers.empty()) {
        _triggers.emplace_back(1000, 1000, 0, 0, true);
    }
}

void LevelManager::_load_traps(const LevelData& level) {
    for (int i = 0; i < level.trap_count; ++i) {
        _traps.push_back(TrapFactory::create(level.traps[i], *this));
    }
}

// -----------------------------------------------------------------------------
// load()
// -----------------------------------------------------------------------------

void LevelManager::load(const LevelData& level) {
    _pause_controller.reset();

    _load_player_spawn(level);
    _load_door(level);
    _load_music(level);
    _load_background(level);
    _setup_camera(level);

    _clear_runtime_state();
    _validate_level(level);

    _load_platforms(level);
    _load_triggers(level);
    _load_traps(level);
}

void LevelManager::unload() {
    _background.reset();
    _door.reset();
    _music.reset();
    _platforms.clear();
    _platform_bodies.clear();
    _triggers.clear();
    _traps.clear();
}

LevelManager::UpdateResult LevelManager::update() {
    // -------------------------------------------------------------------------
    // Pause
    // -------------------------------------------------------------------------

    const auto pause_action = _pause_controller.update();
    if (pause_action == PauseController::Action::DeathRequested) {
        _player.death();
    } else if (
        pause_action == PauseController::Action::ReturnToTitleRequested) {
        // Persist progress before leaving to the title screen.
        _last_death_ct = _player.get_deaths();
        _save_progress();
        return UpdateResult::ReturnToTitle;
    }

    if (_pause_controller.paused()) {
        return UpdateResult::None;
    }

    // -------------------------------------------------------------------------
    // Physics
    // -------------------------------------------------------------------------

    CollisionRegistry::instance().update_all();

    // -------------------------------------------------------------------------
    // Camera
    //
    // Only the foreground (player/platforms/traps/door, via SpriteRegistry)
    // follows the camera each frame. The background stays screen-locked -
    // see the note in _load_background().
    // -------------------------------------------------------------------------

    Camera::instance().follow(_player.pos.x, _player.pos.y);

    // -------------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------------

    SpriteRegistry::instance().sync_all(Camera::instance());
    bn::core::update();

    // -------------------------------------------------------------------------
    // Door Completion
    // -------------------------------------------------------------------------

    if (_door && _door->reached()) {
        return UpdateResult::LevelComplete;
    }

    // -------------------------------------------------------------------------
    // Death Synchronization
    // -------------------------------------------------------------------------

    if (_last_death_ct != _player.get_deaths()) {
        _last_death_ct = _player.get_deaths();
        _save_progress();
        _reset_traps();
    }
    return UpdateResult::None;
}