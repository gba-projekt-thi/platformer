#include "level_manager.h"

LevelManager::LevelManager(Player& player, DataManager& data_manager)
    : _player(player), _pause_controller(), _data_manager(data_manager) {
    // -------------------------------------------------------------------------
    // Restore persistent runtime state
    // -------------------------------------------------------------------------

    Timer& timer = _player.get_timer();

    // Access already-loaded runtime state.
    auto& game_state = _data_manager.state();
    _player.set_deaths(game_state.deaths);
    timer.set_time(game_state.centis, game_state.seconds, game_state.minutes);
}

Trigger& LevelManager::_get_trigger(int trigger_index) {
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
    for (auto& trap : _moving_traps) {
        trap.reset();
    }
    for (auto& trap : _path_traps) {
        trap.reset();
    }
}

void LevelManager::load(const LevelData& level) {
    _pause_controller.reset();

    // -------------------------------------------------------------------------
    // Player Spawn
    // -------------------------------------------------------------------------

    _player.teleport_to(level.player_data.x, level.player_data.y);
    _player.set_spawn_point(level.player_data.x, level.player_data.y);
    _last_death_ct = _player.get_deaths();

    // -------------------------------------------------------------------------
    // Door
    // -------------------------------------------------------------------------

    _door.emplace(level.door.x, level.door.y);

    // -------------------------------------------------------------------------
    // Music
    // -------------------------------------------------------------------------

    if (!_music.has_value() || *_music != level.music) {
        _music.emplace(level.music);
        bn::music::play(*_music);
    }

    // -------------------------------------------------------------------------
    // Background
    // -------------------------------------------------------------------------

    _background.reset();
    _background.emplace(level.back_ground.create_bg(0, 0));
    _background->set_priority(3);
    _background->set_blending_enabled(true);

    // -------------------------------------------------------------------------
    // Clear Previous Level State
    // -------------------------------------------------------------------------

    _platforms.clear();
    _platform_bodies.clear();
    _triggers.clear();
    _base_traps.clear();
    _moving_traps.clear();
    _path_traps.clear();

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    BN_ASSERT(
        unsigned(level.platform_count) < Cfg::Level::Limits::PLATFORMS,
        "Too many platforms");
    BN_ASSERT(
        unsigned(level.trigger_count) < Cfg::Level::Limits::TRIGGERS,
        "Too many triggers");
    unsigned base_trap_count = 0;
    unsigned moving_trap_count = 0;
    unsigned path_trap_count = 0;

    for (int i = 0; i < level.trap_count; ++i) {
        const TrapData& trap = level.traps[i];
        base_trap_count += trap.type == TrapType::BASE;
        moving_trap_count += trap.type == TrapType::MOVING;
        path_trap_count += trap.type == TrapType::PATH;
    }
    BN_ASSERT(
        base_trap_count < Cfg::Level::Limits::BASE_TRAPS,
        "Too many base traps");
    BN_ASSERT(
        moving_trap_count < Cfg::Level::Limits::MOVING_TRAPS,
        "Too many moving traps");
    BN_ASSERT(
        path_trap_count < Cfg::Level::Limits::PATH_TRAPS,
        "Too many path traps");

    // -------------------------------------------------------------------------
    // Platforms
    // -------------------------------------------------------------------------

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

    // -------------------------------------------------------------------------
    // Triggers
    // -------------------------------------------------------------------------

    for (int i = 0; i < level.trigger_count; ++i) {
        const TriggerData& trigger = level.triggers[i];
        _triggers.emplace_back(
            trigger.x, trigger.y, trigger.width, trigger.height);
    }

    // Fallback trigger prevents invalid references.
    if (_triggers.empty()) {
        _triggers.emplace_back(1000, 1000, 0, 0, true);
    }

    // -------------------------------------------------------------------------
    // Trap Construction
    // -------------------------------------------------------------------------

    for (int i = 0; i < level.trap_count; ++i) {
        const TrapData& trap = level.traps[i];
        switch (trap.type) {
            case TrapType::BASE: {
                _base_traps.emplace_back(
                    trap.x, trap.y, trap.width, trap.height, trap.sprite,
                    trap.sprite_waits, trap.graphic_indexes, 0);
                break;
            }
            case TrapType::MOVING: {
                Trigger& trigger = _get_trigger(trap.trigger_index);
                _moving_traps.emplace_back(
                    trap.x, trap.y, trap.width, trap.height, trap.sprite,
                    trap.sprite_waits, trap.graphic_indexes, 0, trap.velocity_x,
                    trap.velocity_y, trap.max_vel, trigger);
                break;
            }
            case TrapType::PATH: {
                Trigger& trigger = _get_trigger(trap.trigger_index);
                _path_traps.emplace_back(
                    trap.x, trap.y, trap.width, trap.height, trap.sprite,
                    trap.sprite_waits, trap.graphic_indexes, 0, trap.path,
                    trap.path_waits, trigger);
                break;
            }
            default:
                BN_LOG(
                    "[ERROR] level_manager: "
                    "unimplemented trap type");
                break;
        }
    }
}

bool LevelManager::update() {
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
        for (int i = 0; i < Cfg::Sleep::DOOR_REACHED; ++i) {
            _door->update();
            bn::core::update();
        }
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