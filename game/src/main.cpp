// Butano
#include "bn_bg_palettes.h"
#include "bn_core.h"

// Engine team
#include "collision_registry.h"
#include "sprite.h"

// Our team
#include "player.h"

// Platforms
#include "bn_sprite_items_platformslevel1.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_ptr.h"

// Sprites
#include "bn_sprite_items_platformslevel1.h"

int main() {
    bn::core::init();

    // Create wallpaper (default)
    bn::regular_bg_ptr bg = bn::regular_bg_items::level1.create_bg(0, 0);

    // Prio 3 to be in the background
    bg.set_priority(3);

    // Create some platforms
    bn::array<bn::sprite_ptr, 5> platform_sprites = {
        bn::sprite_items::platformslevel1.create_sprite(-96, 60),
        bn::sprite_items::platformslevel1.create_sprite(-80, 60),
        bn::sprite_items::platformslevel1.create_sprite(-48, 60),
        bn::sprite_items::platformslevel1.create_sprite(-16, 60),
        bn::sprite_items::platformslevel1.create_sprite(0, 40),

    };

    for (size_t i = 0; i < 4; i++) {
        platform_sprites[i].set_tiles(
            bn::sprite_items::platformslevel1.tiles_item().create_tiles(i % 4));
    }

    bn::array<StaticBody, 5> platform_bodies = {
        StaticBody(-96, 60, 16, 16, Player::PLATFORM_LAYER),
        StaticBody(-80, 60, 16, 16, Player::PLATFORM_LAYER),
        StaticBody(-48, 60, 16, 16, Player::PLATFORM_LAYER),
        StaticBody(-16, 60, 16, 16, Player::PLATFORM_LAYER),
        StaticBody(0, 40, 16, 16, Player::PLATFORM_LAYER),

    };

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