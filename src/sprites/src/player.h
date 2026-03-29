#pragma once

#include "bn_fixed.h"
#include "bn_sprite_ptr.h"

class Player
{
    public:
        Player();
        void update();

    private:
        // Configuration values
        bn::sprite_ptr player_sprite;
        int groundLevel;
        bn::fixed acceleration;      
        bn::fixed max_speed;
        bn::fixed jump_speed;
        bn::fixed gravity;
        bn::fixed max_fall_speed;

        // Working values
        bn::fixed x_coord;
        bn::fixed y_coord;
        bn::fixed x_velocity;
        bn::fixed y_velocity;
        bool canJump;
};