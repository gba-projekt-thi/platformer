#include "bn_bg_palettes.h"
#include "bn_core.h"
#include "bn_common.h"
#include "bn_sprite_text_generator.h"
#include "common_variable_8x16_sprite_font.h"
#include "bn_keypad.h" // testing purpuse 

#include "player.h"
#include "death_counter.h"

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

    // Start Death Counter
    DeathCounter counter;
    bn::sprite_text_generator text_gen(common::variable_8x16_sprite_font);
    DeathCounterHUD hud(text_gen, counter);

    while (true) {
        player.update();

        if (bn::keypad::a_pressed())                // ← statt player_is_dead()
        {
            counter.on_player_death();
        }

        hud.update();
        bn::core::update();
    }
}