#include "level_manager.h"

void LevelManager::startGame(
    const bn::vector<LevelData, 16> levels,
    Player* player) {
    // Store the player object for level updates.
    _player = player;

    // Load and run each level in sequence.
    for (auto level : levels) {
        load(level);
        _run();
    }
}

void LevelManager::load(const LevelData& level) {
    // Position the player and set the respawn point.
    _player->place(level.player_data.x, level.player_data.y);
    _player->set_spawn(level.player_data.x, level.player_data.y);
    _door.emplace(level.door.x, level.door.y);

    // Start the level-specific background music.
    level.music.play();

    _back_ground.emplace(level.back_ground.create_bg(0, 0));
    _back_ground.value().set_priority(3);

    // Clear objects from the previous level before creating the new one.
    _platforms.clear();
    _platform_bodies.clear();
    _triggers.clear();
    _base_traps.clear();
    _moving_traps.clear();

    // Create platform sprites and collision bodies.
    for (int i = 0; i < level.platform_count; i++) {
        const PlatformData& p = level.platforms[i];

        auto sprite = level.sprite_item_platform.create_sprite(p.x, p.y);
        sprite.set_tiles(level.sprite_item_platform.tiles_item().create_tiles(
            p.sprite_index));

        _platforms.push_back(bn::move(sprite));
        _platform_bodies.emplace_back(p.x, p.y, 16, 16, Cfg::Layer::PLATFORM);
    }

    // Create trigger regions that will activate moving traps.
    for (int i = 0; i < level.trigger_count; i++) {
        const TriggerData& t = level.triggers[i];

        _triggers.emplace_back(t.x, t.y, t.width, t.height);
    }

    // Instantiate traps based on level definitions.
    for (int i = 0; i < level.trap_count; i++) {
        const TrapData& t = level.traps[i];

        if (t.type == TrapType::BASE) {
            _base_traps.emplace_back(t.x, t.y, t.width, t.height, t.sprite, 0);
        } else if (t.type == TrapType::MOVING) {
            Trigger& trigger = _triggers[t.trigger_index];
            _moving_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, 0, t.velocity_x,
                t.velocity_y, t.max_vel, t.range, trigger);
        }
    }
}

void LevelManager::_run() {
    bn::fixed last_death_ct = _player->get_deaths();

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
        }
    }
}