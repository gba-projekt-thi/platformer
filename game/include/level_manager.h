#pragma once

#include "bn_core.h"
#include "bn_dmg_music_item.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "door.h"
#include "level_structure.h"
#include "moving_trap.h"
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
    bn::vector<bn::sprite_ptr, 32> _platforms;
    bn::vector<StaticBody, 32> _platform_bodies;
    bn::vector<Trigger, 16> _triggers;
    bn::vector<BaseTrap, 16> _base_traps;
    bn::vector<MovingTrap, 16> _moving_traps;
    bn::optional<bn::regular_bg_ptr> _back_ground;
    Player* _player;
    bn::optional<Door> _door;
    // Main loop for the currently loaded level.
    void _run();
};