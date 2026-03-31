#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_sprite_items_ente.h"   // verwendetes sprite
#include "bn_bg_palettes.h"

#include "bn_sprites_mosaic.h"
#include "bn_sprite_actions.h"
#include "bn_sprite_builder.h"
#include "bn_sprites_actions.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_first_attributes.h"
#include "bn_sprite_third_attributes.h"
#include "bn_sprite_position_hbe_ptr.h"
#include "bn_sprite_first_attributes_hbe_ptr.h"
#include "bn_sprite_third_attributes_hbe_ptr.h"
#include "bn_sprite_affine_second_attributes.h"
#include "bn_sprite_regular_second_attributes.h"
#include "bn_sprite_affine_second_attributes_hbe_ptr.h"
#include "bn_sprite_regular_second_attributes_hbe_ptr.h"

#include "bn_sprite_items_ninja.h"
#include "bn_sprite_items_caveman.h"
#include "bn_sprite_items_red_sprite.h"
#include "bn_sprite_items_blue_sprite.h"
#include "bn_sprite_items_green_sprite.h"
#include "bn_sprite_items_yellow_sprite.h"
#include "bn_regular_bg_items_red_bg.h"
#include "bn_regular_bg_items_blue_bg.h"
#include "bn_regular_bg_items_green_bg.h"
#include "bn_regular_bg_items_yellow_bg.h"

int main()
{
    bn::core::init();

    // Sprite wird erstellt
    bn::sprite_ptr ente_sprite = bn::sprite_items::ente.create_sprite(0, 0);

    // hintergrund farbe
    // grau     16,16,16
    // blau     00,00,16
    //      
    bn::bg_palettes::set_transparent_color(bn::color(12,20,31)); 
    // für animation
    bn::sprite_animate_action<2> action = bn::create_sprite_animate_action_forever(
                    ente_sprite, 26, bn::sprite_items::ente.tiles_item(), 0, 1);
    // var für bewegungs-check
    // 0 = nichts
    // 1 = rechts
    // -1 = links
    int direction = 0;
    
    while(!bn::keypad::start_pressed())
    {
        bool moving = false;

        // Bewegung
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
            // nur sprite anpassen
            ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item().create_tiles(9));
        }
        if(bn::keypad::down_held())
        {
            direction = 0;
            // nur sprite anpassen
            ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item().create_tiles(8));
        }
        
        if (moving) {
            action.update();
        } else {
            // tiles nach bewegung setzen
            if(direction == -1) {
                ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item(), 4);
            } else if (direction == 1) {
                ente_sprite.set_tiles(bn::sprite_items::ente.tiles_item(), 0);
            } else if (direction == 0) {
                // nichts machen
            }
        }

            

        bn::core::update();
    }
}