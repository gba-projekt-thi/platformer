#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"

#include "bn_sprite_animate_actions.h"

// Sprites
#include "bn_sprite_items_ente.h"
#include "bn_sprite_items_platformslevel1.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_ptr.h"

int main()
{
    bn::core::init();

    // Create wallpaper
    bn::regular_bg_ptr bg = bn::regular_bg_items::level1.create_bg(0, 0);
    
    // Prio 3 to be in the background
    bg.set_priority(3);

    // Create character sprite
    bn::sprite_ptr ente_sprite = bn::sprite_items::ente.create_sprite(0, 0);
    
    // This is how to add sprites, just change the (0,0) to (16,0) etc.
    bn::sprite_ptr platformlevel1_sprite = bn::sprite_items::platformslevel1.create_sprite(0, 0);

    // Animation
    bn::sprite_animate_action<2> action = bn::create_sprite_animate_action_forever(
                    ente_sprite, 26, bn::sprite_items::ente.tiles_item(), 0, 1);
    // var for movement-check
    // 0 = nothing
    // 1 = right
    // -1 = left
    int direction = 0;
    
    while(!bn::keypad::start_pressed())
    {
        bool moving = false;

        // Movement
        if(bn::keypad::left_held())
        {       
            ente_sprite.set_x(ente_sprite.x() - 1);
            if(direction != -1) 
            {
                action = bn::create_sprite_animate_action_forever(
                ente_sprite, 10, bn::sprite_items::ente.tiles_item(), 4, 5);
                direction = -1;
            }
            moving = true;
        }
        if(bn::keypad::right_held())
        {            
            ente_sprite.set_x(ente_sprite.x() + 1);
            if(direction != 1) 
            {
                action = bn::create_sprite_animate_action_forever(
                ente_sprite, 10, bn::sprite_items::ente.tiles_item(), 0, 1);
                direction = 1;
            }
            moving = true;
        }

        if(bn::keypad::up_held())
        {
            direction = 0;
            // Change sprite only
            ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item().create_tiles(9));
        }
        if(bn::keypad::down_held())
        {
            direction = 0;
            // Change sprite only
            ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item().create_tiles(8));
        }
        
        if (moving) {
            action.update();
        } else {
            // Set tiles after movement
            if(direction == -1) {
                ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item(), 4);
            } else if (direction == 1) {
                ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item(), 0);
            } else if (direction == 0) {
                // do nothing
            }
        }

        bn::core::update();
    }
}