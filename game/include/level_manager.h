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
    void startGame(const bn::vector<LevelData, 16> levels, Player* player);
    // Load a single level into memory and initialize its entities.
    void load(const LevelData& level);

   private:
    // TODO: figure out sizes that are reasonable
    bn::vector<bn::sprite_ptr, Cfg::Level::Limits::PLATFORMS> _platforms;
    bn::vector<StaticBody, Cfg::Level::Limits::PLATFORM_BODIES>
        _platform_bodies;
    bn::vector<Trigger, Cfg::Level::Limits::TRIGGERS> _triggers;
    bn::vector<BaseTrap, Cfg::Level::Limits::BASE_TRAPS> _base_traps;
    bn::vector<MovingTrap, Cfg::Level::Limits::MOVING_TRAPS> _moving_traps;
    bn::vector<PathTrap, Cfg::Level::Limits::PATH_TRAPS> _path_traps;
    bn::optional<bn::regular_bg_ptr> _back_ground;
    Player* _player;
    bn::optional<Door> _door;
    bn::optional<bn::music_item> _music;
    bn::vector<bn::sprite_ptr, 64> _pause_sprites;
    // Main loop for the currently loaded level.
    void _run();
};