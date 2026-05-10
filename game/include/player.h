#pragma once

#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_items_ente.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_fixed_8x16_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"

#include "cfg.h"
#include "death_counter.h"
#include "physics_body.h"
#include "sprite.h"
#include "timer.h"

class Player : public PhysicsBody {
   public:
    // Collision layers for the player and other objects.
    static constexpr uint16_t BLOCK =
        0xFFFF & ~Cfg::Layer::TRAP & ~Cfg::Layer::TRIGGER;

    Player(
        bn::fixed in_start_x,
        bn::fixed in_start_y,
        bn::fixed in_width,
        bn::fixed in_height);

    // Main per-frame update for player motion and animation.
    void update() override;

    // Handle player death and respawn.
    void death();

    // Set the current respawn location.
    void set_spawn_point(bn::fixed in_x, bn::fixed in_y);
    // teleport the player immediately to the given coordinate.
    void teleport_to(bn::fixed in_x, bn::fixed in_y);
    // Get the count of player deaths.
    unsigned int get_deaths() const;

   private:
    // Sprite
    Sprite player_sprite;

    // Cached tile handles (zero allocation at runtime)
    bn::vector<bn::sprite_tiles_ptr, Cfg::Player::PLAYER_TILE_CACHE_SIZE>
        cached_tiles;

    // Walk animation action
    bn::sprite_animate_action<Cfg::Player::ANIMATION_FRAME_COUNT> walk_action;

    // Jump animation action
    bn::sprite_animate_action<Cfg::Player::ANIMATION_FRAME_COUNT> jump_action;

    // Previous movement state tracking
    bool wasMoving = false;
    bool wasJumping = false;

    // Death counter UI
    DeathCounter deathCounter;
    bn::sprite_text_generator deathCounterTextGen;
    DeathCounterHUD deathCounterHud;

    // Timer UI
    Timer timer;
    bn::sprite_text_generator timerTextGen;
    TimerHUD timerHud;

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

    // Jump helpers
    int coyote_timer = 0;
    int jump_buffer_timer = 0;

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
};