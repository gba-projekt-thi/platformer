#include "player.h"

#include "bn_keypad.h"
#include "bn_sprite_items_caveman.h"
#include "bn_log.h"

Player::Player() :

    // Configuration values
    player_sprite(bn::sprite_items::caveman.create_sprite(0,0)),
    groundLevel(60),
    acceleration(0.5),
    max_speed(3),
    jump_speed(-4), // This should be negative
    gravity(0.2),
    max_fall_speed(3),
    
    // Working values
    x_coord(0),
    y_coord(groundLevel),
    x_velocity(0),
    y_velocity(0),
    canJump(true)
{
    player_sprite.set_scale(0.5);
}

void Player::update()
{
    // Handle player input for left and right
    if(bn::keypad::left_held())
    {
        x_velocity -= acceleration;
        if(x_velocity < -max_speed){
            x_velocity = -max_speed;
        }
    }
    else if(bn::keypad::right_held())
    {
        x_velocity += acceleration;
        if(x_velocity > max_speed){
            x_velocity = max_speed;
        }
    }
    else {
        // Slow down when there is no input
        if(x_velocity < 0){
            x_velocity += acceleration;
            if(x_velocity > 0) x_velocity = 0;
        }
        else if(x_velocity > 0){
            x_velocity -= acceleration;
            if(x_velocity < 0) x_velocity = 0;
        }
    }

    // Calculate new x_coordinate
    x_coord += x_velocity;
    
    // Check if we're at the edge of the map
    if(x_coord < -110)
    {
        x_coord = -110;
        x_velocity = 0;
    }
    else if(x_coord > 110)
    {
        x_coord = 110;
        x_velocity = 0;
    }
    

    // Jump
    if(bn::keypad::a_pressed() && canJump)
    {
        y_velocity = jump_speed;
        canJump = false;
    }

    // Gravity
    y_velocity += gravity;
    if(y_velocity > max_fall_speed){
        y_velocity = max_fall_speed;
    }

    // Calculate new y_coordinate
    y_coord += y_velocity;
    
    // Check if we're on the Ground
    if(y_coord >= groundLevel){
        y_coord = groundLevel;
        y_velocity = 0;
        canJump = true;
    }


    // Update player sprite
    player_sprite.set_position(x_coord, y_coord);

    BN_LOG("Player x:", x_coord, " y:", y_coord);
}
