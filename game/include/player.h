#pragma once

#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_items_ente.h"
#include "bn_sprite_text_generator.h"
#include "common_variable_8x16_sprite_font.h"

#include "death_counter.h"
#include "physics_body.h"
#include "sprite.h"

class Player : public PhysicsBody {
   public:
    static constexpr uint16_t LAYERS = 0x0001;
    static constexpr uint16_t TRAP_LAYER = 0x0002;
    static constexpr uint16_t PLATFORM_LAYER = 0x0004;
    static constexpr uint16_t TRIGGER_LAYER = 0x0008;

    static constexpr uint16_t MASK = TRAP_LAYER;  // detect
    static constexpr uint16_t BLOCK = 0xFFFF & ~TRAP_LAYER & ~TRIGGER_LAYER;

    static constexpr int DEFAULT_DEATH_HEIGHT = 100;

    Player(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height);

    void update() override;
    void death();

   private:
    // Sprite and animation
    Sprite player_sprite;
    bn::sprite_animate_action<2> action;

    // Death counter UI
    DeathCounter deathCounter;
    bn::sprite_text_generator deathCounterTextGen;
    DeathCounterHUD deathCounterHud;

    // Respawn position
    bn::fixed restart_x;
    bn::fixed restart_y;

    // Physics parameters
    bn::fixed acceleration;
    bn::fixed max_speed;
    bn::fixed jump_speed;
    bn::fixed gravity;
    bn::fixed max_fall_speed;
    int deathHeight;

    // Ground state
    bool onGround;

    // Facing direction
    enum class Facing { Forward, Back, Left, Right };
    Facing facing;

    // State machine
    enum class PlayerState { Idle, Run, Jump, Fall };
    PlayerState state;

    // Animation frames
    static constexpr int LEFT_FRAMES[2] = {4, 5};
    static constexpr int RIGHT_FRAMES[2] = {0, 1};
    static constexpr int IDLE_FRAME = 8;
    static constexpr int BACK_FRAME = 9;

    // Jump helpers
    int coyote_timer = 0;
    int jump_buffer_timer = 0;

    static constexpr int COYOTE_FRAMES = 4;
    static constexpr int JUMP_BUFFER_FRAMES = 6;

    // Input & physics handlers
    void handle_horizontal_input();
    void handle_jump();
    void apply_gravity();
    void apply_variable_jump();
    void clamp_velocity();
    void check_bounds();
    void update_ground_state();
    void check_death();

    // State machine
    void update_state();
    void enter_state(PlayerState new_state);

    // Animation helpers
    void update_animation();
    void set_direction(Facing new_facing, int tile_index);
    void set_frame(int tile_index);
    void set_walk_animation(const int frames[2]);
};