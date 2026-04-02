#include "bn_bg_palettes.h"
#include "bn_core.h"

#include "player.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_ptr.h"

int main() {
    bn::core::init();

    // Create wallpaper
    bn::regular_bg_ptr bg = bn::regular_bg_items::level1.create_bg(0, 0);

    // Prio 3 to be in the background
    bg.set_priority(3);

    // Create player
    Player player;

    while (true) {
        player.update();
        bn::core::update();
    }
}