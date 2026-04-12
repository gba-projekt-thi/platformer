#pragma once

// Butano
#include "bn_fixed.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"

// Engine team
#include "physics_body.h"
#include "sprite.h"

class Player : public PhysicsBody {
   public:
    static constexpr uint16_t LAYERS = 0x0001;  // what am I
    static constexpr uint16_t TRAP_LAYER = 0x0002;
    static constexpr uint16_t PLATFORM_LAYER = 0x0004;

    static constexpr uint16_t MASK =
        TRAP_LAYER;  // what I detect (on_enter / on_exit)
    static constexpr uint16_t BLOCK =
        0xFFFF & ~TRAP_LAYER;  // what blocks me (pass through traps)

    Player(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height);
    void update() override;

   private:
    // Used for sprite animation
    enum class Facing { Forward, Back, Left, Right };

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
    bn::sprite_animate_action<2> action;
    Facing facing;
};