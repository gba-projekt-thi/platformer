// Butano
#include "bn_bg_palettes.h"
#include "bn_core.h"
#include "bn_vector.h"

// Engine team
#include "collision_registry.h"
#include "sprite.h"

// Our team
#include "moving_trap.h"
#include "player.h"
#include "trigger.h"

// Level 1
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_door32x32.h"
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_platformslvl1.h"

// Level 2
#include "bn_sprite_items_box32x32.h"
#include "bn_sprite_items_can32x32.h"
#include "bn_sprite_items_ladder.h"
#include "bn_sprite_items_pipe16x16.h"
#include "bn_sprite_items_platformslvl2.h"
#include "bn_sprite_items_rustynails32x16.h"

// Level 3
#include "bn_sprite_items_baumstamm.h"
#include "bn_sprite_items_branch32x16.h"
#include "bn_sprite_items_mushroom32x32.h"
#include "bn_sprite_items_nebel.h"
#include "bn_sprite_items_slithermanarms32x32.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_items_level3.h"
#include "bn_regular_bg_ptr.h"

// Music
#include "bn_music_items.h"

// Levels
#include "level_manager.h"
#include "levels.h"

int main() {
    // Initialize the Butano engine core before using any rendering or input
    // features.
    bn::core::init();

    // Create the ordered level list for game progression.
    bn::vector<LevelData, 16> levels;
    levels.push_back(LEVEL_0);
    levels.push_back(LEVEL_1);
    levels.push_back(LEVEL_2);
    levels.push_back(LEVEL_3);

    // Create the player entity and apply an offset for the sprite anchor point.
    Player player(0, 0, 8, 8);

    // Initialize the level manager and begin the game loop.
    static DataManager data_manager;
    static LevelManager level_manager
        __attribute__((section(".ewram"))) (data_manager);
    // move to ewram(slower, more space(=> more platforms tho?!)) to save stack
    // usage
    level_manager.startGame(levels, &player);
}
