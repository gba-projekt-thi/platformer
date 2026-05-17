#pragma once

#include "bn_core.h"
#include "bn_dmg_music_item.h"
#include "bn_music.h"
#include "bn_music_items.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "cfg.h"
#include "door.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"
#include "player.h"
#include "trigger.h"

class LevelManager {
   public:
    // Start the game and sequentially run the provided levels.
    void start_game(const bn::vector<LevelData, 16> levels, Player* player);
    // Load a single level into memory and initialize its entities.
    void load(const LevelData& level);

   private:
    // TODO: figure out sizes that are reasonable
    bn::vector<bn::sprite_ptr, Cfg::Level::Limits::PLATFORMS> platforms;
    bn::vector<StaticBody, Cfg::Level::Limits::PLATFORM_BODIES> platform_bodies;
    bn::vector<Trigger, Cfg::Level::Limits::TRIGGERS> triggers;
    bn::vector<BaseTrap, Cfg::Level::Limits::BASE_TRAPS> base_traps;
    bn::vector<MovingTrap, Cfg::Level::Limits::MOVING_TRAPS> moving_traps;
    bn::vector<PathTrap, Cfg::Level::Limits::PATH_TRAPS> path_traps;
    bn::optional<bn::regular_bg_ptr> back_ground;
    Player* player_ptr;
    bn::optional<Door> door_ptr;
    bn::optional<bn::music_item> music_ptr;
    bn::vector<bn::sprite_ptr, 64> pause_sprites;

    // Main loop for the currently loaded level.
    void run();
};