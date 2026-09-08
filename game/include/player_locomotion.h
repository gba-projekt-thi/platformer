#pragma once

#include "bn_fixed.h"

#include "cfg.h"
#include "physics_body.h"
#include "sprite.h"

// -----------------------------------------------------------------------------
// PlayerDeathHandler
//
// Callback interface so PlayerLocomotion can trigger a death exactly at the
// point in the frame the original Player::update() did (before ground state
// / jump handling run), without owning respawn/HUD/sound logic itself.
// -----------------------------------------------------------------------------
class PlayerDeathHandler {
   public:
    virtual void on_locomotion_death() = 0;

   protected:
    ~PlayerDeathHandler() = default;
};

// -----------------------------------------------------------------------------
// PlayerLocomotion
//
// Owns horizontal/vertical movement, jump buffering + coyote time, ground
// detection, screen-bounds bouncing, and the fall-death check. Extracted out
// of Player as the fourth (final) step of the Player decomposition (after
// PlayerHud, PlayerAnimator, PlayerStateMachine).
// -----------------------------------------------------------------------------
class PlayerLocomotion {
   public:
    enum class Facing { Forward, Back, Left, Right };

    PlayerLocomotion(
        PhysicsBody& body,
        Sprite& sprite,
        uint16_t block_mask,
        PlayerDeathHandler& death_handler);

    // Full per-frame physics/input step, in the same order the original
    // Player::update() ran it (including the mid-sequence death check).
    void update();

    bool on_ground() const { return _on_ground; }
    Facing facing() const { return _facing; }

   private:
    void handle_horizontal_input();
    void handle_jump();
    void apply_gravity();
    void apply_variable_jump();
    void clamp_velocity();
    void check_bounds();
    void update_ground_state();
    void check_death();

    PhysicsBody& _body;
    Sprite& _sprite;
    uint16_t _block_mask;
    PlayerDeathHandler& _death_handler;

    // Physics parameters
    bn::fixed _acceleration = Cfg::Player::ACCELERATION;
    bn::fixed _max_speed = Cfg::Player::MAX_SPEED;
    bn::fixed _jump_speed = Cfg::Player::JUMP_SPEED;
    bn::fixed _gravity = Cfg::Player::GRAVITY;
    bn::fixed _max_fall_speed = Cfg::Player::MAX_FALL_SPEED;
    int _death_height = Cfg::Player::DEATH_HEIGHT;

    // Ground state
    bool _on_ground = true;

    // Facing direction
    Facing _facing = Facing::Forward;

    // Jump helpers
    int _coyote_timer = 0;
    int _jump_buffer_timer = 0;

    // Walk sound helper
    int _walk_sound_counter = 0;
};