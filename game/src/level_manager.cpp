#include "level_manager.h"

void LevelManager::start_game(
    const bn::vector<LevelData, 16> levels,
    Player* player) {
    // Store the player object for level updates.
    player_ptr = player;

    pause_sprites.clear();
    bn::sprite_text_generator text_gen(common::variable_8x16_sprite_font);
    text_gen.set_z_order(Cfg::ZOrder::PAUSE_MENU);  // total foreground
    text_gen.generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_0, "Paused", pause_sprites);
    text_gen.generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_1, "Continue: Start",
        pause_sprites);
    text_gen.generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_2, "Die: Select", pause_sprites);

    // hide menu
    for (bn::sprite_ptr& sprite : pause_sprites)
        sprite.set_visible(false);

    // Load and run each level in sequence.
    for (const auto& level : levels) {
        load(level);
        run();
    }

    for (int i = 0; i < Cfg::Sleep::FINISHED_GAME; ++i)  // sleep 2s
        bn::core::update();
}

void LevelManager::load(const LevelData& level) {
    // Position the player and set the respawn point.
    player_ptr->teleport_to(level.player_data.x, level.player_data.y);
    player_ptr->set_spawn_point(level.player_data.x, level.player_data.y);
    door_ptr.emplace(level.door.x, level.door.y);

    // Start the level-specific background music.
    music_ptr.emplace(level.music);
    bn::music::play(*music_ptr);

    back_ground.reset();
    back_ground.emplace(level.back_ground.create_bg(0, 0));
    back_ground.value().set_priority(3);

    // Clear objects from the previous level before creating the new one.
    platforms.clear();
    platform_bodies.clear();
    triggers.clear();
    base_traps.clear();
    moving_traps.clear();
    path_traps.clear();

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
        count_base_traps += (level.traps[i].type == TrapType::Base);
        count_moving_traps += (level.traps[i].type == TrapType::Moving);
        count_path_traps += (level.traps[i].type == TrapType::Path);
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

        int tile_count =
            p.sprite.tiles_item()
                .graphics_count();  // to check for out of bound errors

        auto sprite = p.sprite.create_sprite(p.x, p.y);
        sprite.set_tiles(
            p.sprite.tiles_item().create_tiles(p.sprite_index % tile_count));

        platforms.push_back(bn::move(sprite));
        platform_bodies.emplace_back(
            p.x, p.y, platforms.back().dimensions().width(),
            platforms.back().dimensions().height(), Cfg::Layer::PLATFORM);
    }

    // Create trigger regions that will activate moving traps.
    for (int i = 0; i < level.trigger_count; i++) {
        const TriggerData& t = level.triggers[i];
        triggers.emplace_back(t.x, t.y, t.width, t.height);
    }

    // add one trigger that's always true in case of no trigger as a fallback
    if (!triggers.size())
        triggers.emplace_back(1000, 1000, 0, 0, true);

    // Instantiate traps based on level definitions.
    for (int i = 0; i < level.trap_count; i++) {
        const TrapData& t = level.traps[i];

        if (t.type == TrapType::Base) {
            base_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, t.sprite_waits,
                t.graphic_indexes, 0);
        } else if (t.type == TrapType::Moving) {
            Trigger& trigger =
                (t.trigger_index >= 0 && t.trigger_index < triggers.size())
                    ? triggers[t.trigger_index]
                    : triggers[0];

            if (!(t.trigger_index >= 0 && t.trigger_index < triggers.size()))
                BN_LOG(
                    "[ERROR] level_manager: no valid trigger for trap found, "
                    "fallback to trigger 0");

            moving_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, t.sprite_waits,
                t.graphic_indexes, 0, t.velocity_x, t.velocity_y, t.max_vel,
                t.range, trigger);
        } else if (t.type == TrapType::Path) {
            Trigger& trigger =
                (t.trigger_index >= 0 && t.trigger_index < triggers.size())
                    ? triggers[t.trigger_index]
                    : triggers[0];

            if (!(t.trigger_index >= 0 && t.trigger_index < triggers.size()))
                BN_LOG(
                    "[ERROR] level_manager: no valid trigger for trap found, "
                    "fallback to trigger 0");

            path_traps.emplace_back(
                t.x, t.y, t.width, t.height, t.sprite, t.sprite_waits,
                t.graphic_indexes, 0, t.path, t.path_waits, trigger);
        } else {
            BN_LOG(
                "[ERROR] level_manager: trap variant not implemented, don't "
                "forget the resets");
        }
    }
}

void LevelManager::run() {
    unsigned int last_death_count = player_ptr->get_deaths();
    bool paused = false;
    bool prev_paused = paused;

    while (true) {
        if (bn::keypad::start_released()) {
            paused = !paused;
        }

        if (prev_paused != paused) {
            prev_paused = paused;
            for (auto& sprite : pause_sprites)
                sprite.set_visible(paused);
            if (paused)
                bn::music::pause();
            else
                bn::music::resume();
        }

        if (paused) {
            if (bn::keypad::select_released()) {
                player_ptr->death();
                paused = false;
            } else {
                bn::core::update();
                continue;
            }
        }

        // Physics update
        CollisionRegistry::instance().update_all();
        SpriteRegistry::instance().sync_all(Camera::instance());
        bn::core::update();

        if (door_ptr && door_ptr->reached()) {
            for (int i = 0; i < Cfg::Sleep::DOOR_REACHED; ++i) {
                door_ptr->update();
                bn::core::update();
            }

            break;
        }

        // check for death and reset traps if detected
        if (last_death_count != player_ptr->get_deaths()) {
            last_death_count = player_ptr->get_deaths();
            for (auto& moving_trap : moving_traps) {
                moving_trap.reset();
            }
            for (auto& path_trap : path_traps) {
                path_trap.reset();
            }
        }
    }
}