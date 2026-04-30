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

// Platforms
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_door32x32.h"
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_platformslvl1.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_ptr.h"

// Music
#include "bn_dmg_music_items_level1.h"
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

    // Create the player entity and apply an offset for the sprite anchor point.
    Player player(0, 0, 8, 8);
    player.sprite->pos.offset_y = 4;

    // Initialize the level manager and begin the game loop.
    LevelManager level_manager;
    level_manager.startGame(levels, &player);
}
