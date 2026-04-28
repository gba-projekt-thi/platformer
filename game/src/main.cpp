// Butano
#include "bn_bg_palettes.h"
#include "bn_core.h"

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
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_platformslvl1.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_ptr.h"

// Music
#include "bn_dmg_music_items_level1.h"

// Levels
#include "level_manager.h"
#include "levels.h"

int main() {
    bn::core::init();

    // Init lvl mngr
    LevelManager level_manager;
    level_manager.load(LEVEL_1);

    // Create player
    Player player(-96, 0, 8, 8);
    player.sprite_offset_y = 4;

    while (true) {
        // Physics update
        CollisionRegistry::instance().update_all();

        // Camera should not follow the player for now
        // Camera::instance().follow(player.x, player.y);

        SpriteRegistry::instance().sync_all(Camera::instance());

        bn::core::update();
    }
}