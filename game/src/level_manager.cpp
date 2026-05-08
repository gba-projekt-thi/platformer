#include "level_manager.h"

void LevelManager::startGame(
    const bn::vector<LevelData, 16> levels,
    Player* player) {
    // Store the player object for level updates.
    _player = player;

    // Load and run each level in sequence.
    for (const auto& level : levels) {
        load(level);
        _run();
    }
}

void LevelManager::load(const LevelData& level) {
    // Position the player and set the respawn point.
    _player->teleport_to(level.player_data.x, level.player_data.y);
    _player->set_spawn_point(level.player_data.x, level.player_data.y);
    _door.emplace(level.door.x, level.door.y);

    // Start the level-specific background music.
    level.music.play();

    _back_ground.reset();
    _back_ground.emplace(level.back_ground.create_bg(0, 0));
    _back_ground.value().set_priority(3);

    // Clear objects from the previous level before creating the new one.
    _platforms.clear();
    _platform_bodies.clear();
    _triggers.clear();
    _base_traps.clear();
    _moving_traps.clear();
    _path_traps.clear();

    BN_ASSERT(
        static_cast<unsigned int>(level.platform_count) <
            Cfg::Level::Limits::PLATFORMS,
        "Too many platforms");
    BN_ASSERT(
        static_cast<unsigned int>(level.trigger_count) <
            Cfg::Level::Limits::TRIGGERS,
        "Too many triggers");

    unsigned int count_base_traps = 0;
    unsigned int count_moving_traps = 0;
    unsigned int count_path_traps = 0;
    for (int i = 0; i < level.trap_count; i++) {
        count_base_traps += (level.traps[i].type == TrapType::BASE);
        count_moving_traps += (level.traps[i].type == TrapType::MOVING);
        count_path_traps += (level.traps[i].type == TrapType::PATH);
    }
    BN_ASSERT(
        count_base_traps < Cfg::Level::Limits::BASE_TRAPS,
        "Too many base traps");
    BN_ASSERT(
        count_moving_traps < Cfg::Level::Limits::MOVING_TRAPS,
        "Too many moving traps");
    BN_ASSERT(
        count_path_traps < Cfg::Level::Limits::PATH_TRAPS,
        "Too many path traps");

    // Create platform sprites and collision bodies.
    for (int i = 0; i < level.platform_count; i++) {
        const PlatformData& p = level.platforms[i];

        int count = p.sprite.tiles_item().graphics_count(); // to check for out of bound errors

        auto sprite = p.sprite.create_sprite(p.x, p.y);
        sprite.set_tiles(p.sprite.tiles_item().create_tiles(
            p.sprite_index % count));

        _platforms.push_back(bn::move(sprite));
        _platform_bodies.emplace_back(p.x, p.y,_platforms.back().dimensions().width(), _platforms.back().dimensions().height() , Cfg::Layer::PLATFORM);
    }

    // Create trigger regions that will activate moving traps. //must before any
    // refrencing also no touching it afterwards!
    for (int i = 0; i < level.trigger_count; i++) {
        const TriggerData& t = level.triggers[i];

        _triggers.emplace_back(t.x, t.y, t.width, t.height);
    }

    // add one trigger thats always true in case of no trigger as a fallback
    if (!_triggers.size())
        _triggers.emplace_back(1000, 1000, 0, 0, true);

    // Instantiate traps based on level definitions.
    for (int i = 0; i < level.trap_count; i++) {
        const TrapData& t = level.traps[i];

        if (t.type == TrapType::BASE) {
            _base_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, t.sprite_waits,
                t.graphic_indexes, 0);
        } else if (t.type == TrapType::MOVING) {
            // fallback to first trigger if invalid index
            Trigger& trigger =
                (t.trigger_index >= 0 && t.trigger_index < _triggers.size())
                    ? _triggers[t.trigger_index]
                    : _triggers[0];

            if (!(t.trigger_index >= 0 && t.trigger_index < _triggers.size()))
                BN_LOG(
                    "[ERROR] level_manager: no valid trigger for trap found "
                    "fallback to trigger 0");

            _moving_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, t.sprite_waits,
                t.graphic_indexes, 0, t.velocity_x, t.velocity_y, t.max_vel,
                t.range, trigger);
        } else if (t.type == TrapType::PATH) {
            // fallback to first trigger if invalid index
            Trigger& trigger =
                (t.trigger_index >= 0 && t.trigger_index < _triggers.size())
                    ? _triggers[t.trigger_index]
                    : _triggers[0];

            if (!(t.trigger_index >= 0 && t.trigger_index < _triggers.size()))
                BN_LOG(
                    "[ERROR] level_manager: no valid trigger for trap found "
                    "fallback to trigger 0");

            _path_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, t.sprite_waits,
                t.graphic_indexes, 0, t.path, t.path_waits, trigger);
        } else {
            BN_LOG(
                "[ERROR] level_manager: trap variant not implemented, dont "
                "forget the resets");
        }
    }
}

void LevelManager::_run() {
    unsigned int last_death_ct = _player->get_deaths();

    while (true) {
        // Physics update
        CollisionRegistry::instance().update_all();
        // Camera should not follow the player for now
        // Camera::instance().follow(player.x, player.y);
        SpriteRegistry::instance().sync_all(Camera::instance());
        bn::core::update();

        if (_door && _door->reached()) {
            break;
        }

        // check for death and reset traps if detcted
        if (last_death_ct != _player->get_deaths()) {
            last_death_ct = _player->get_deaths();
            for (auto& mv_trap : _moving_traps) {
                mv_trap.reset();
            }
            for (auto& pth_trap : _path_traps) {
                pth_trap.reset();
            }
        }
    }
}