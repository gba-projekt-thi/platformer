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

    // Updates the horizontal screen-bounce boundary to match the current
    // level's world width (world_width/2 - Cfg::Screen::EDGE_MARGIN).
    // Called by LevelManager::load() whenever a new level starts.
    void set_horizontal_bound(bn::fixed bound) { _horizontal_bound = bound; }

    // Scales braking-only deceleration for the current level (1 = normal
    // stopping, <1 = a slippery/icy floor that skids further before
    // stopping). Called by LevelManager::load() with that level's
    // LevelData::ground_friction. Leaves acceleration, gravity, and jump
    // speed untouched, so it never changes how far a jump can reach.
    void set_ground_friction(bn::fixed scale) {
        _ground_friction_scale = scale;
    }

    // Assist Mode: swaps coyote time and jump buffer to the wider
    // Cfg::AssistMode windows. Only forgiveness timing changes - jump
    // speed, gravity and acceleration stay untouched, so the jump
    // envelope level layouts are authored against is unaffected. Called
    // by LevelManager::load() on every level load.
    // Dash (B): enabled per level by LevelManager::load(). Changing the
    // flag also cancels a running dash and refills the air dash.
    void set_dash_enabled(bool enabled) {
        _dash_enabled = enabled;
        reset_dash();
    }

    // Cancels a running dash and clears its cooldown/air-use (called on
    // death and on level load so a dash never carries over a respawn).
    void reset_dash() {
        _dashing = false;
        _dash_timer = 0;
        _dash_cooldown_timer = 0;
        _dash_available = true;
    }

    // True while a dash is in progress (read by Player for animation).
    bool dashing() const { return _dashing; }

    void set_assist_mode(bool enabled) {
        _coyote_frames = enabled ? Cfg::AssistMode::COYOTE_FRAMES
                                 : Cfg::Player::COYOTE_FRAMES;
        _jump_buffer_frames = enabled ? Cfg::AssistMode::JUMP_BUFFER_FRAMES
                                      : Cfg::Player::JUMP_BUFFER_FRAMES;
    }

   private:
    void handle_horizontal_input();
    void handle_jump();
    void apply_gravity();
    void apply_variable_jump();
    void clamp_velocity();
    void check_bounds();
    void update_ground_state();
    void check_death();
    // Per-frame dash step. Returns true when a dash owns this frame (the
    // normal movement path must then be skipped); false otherwise, also
    // on the frame a dash ends so normal movement resumes immediately.
    bool update_dash();
    void start_dash();
    void end_dash();

    PhysicsBody& _body;
    Sprite& _sprite;
    uint16_t _block_mask;
    PlayerDeathHandler& _death_handler;

    // Physics parameters
    bn::fixed _acceleration = Cfg::Player::ACCELERATION;
    // Braking constant, separate from _acceleration - see the comment on
    // Cfg::Player::DECELERATION. Scaled per level by _ground_friction_scale.
    bn::fixed _deceleration = Cfg::Player::DECELERATION;
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
    // Window lengths (frames) - Cfg::Player defaults, widened by
    // set_assist_mode().
    int _coyote_frames = Cfg::Player::COYOTE_FRAMES;
    int _jump_buffer_frames = Cfg::Player::JUMP_BUFFER_FRAMES;

    // Dash state. _dash_available is the air-dash charge: refilled on real
    // ground contact (not coyote time), spent by start_dash().
    bool _dash_enabled = false;
    bool _dashing = false;
    bool _dash_available = true;
    int _dash_dir = 1;  // -1 = left, +1 = right
    int _dash_timer = 0;
    int _dash_cooldown_timer = 0;

    // Walk sound helper
    int _walk_sound_counter = 0;

    // Horizontal screen-bounce boundary; updated per level by LevelManager
    // to match that level's world_width (defaults to the legacy 240-wide
    // screen bound so a freshly-constructed Player behaves as before).
    bn::fixed _horizontal_bound = Cfg::Screen::HORIZONTAL_EDGE;

    // Multiplier applied to _deceleration only (braking while grounded);
    // never touches _acceleration, gravity, or jump speed, so it cannot
    // shrink the jump envelope levels are authored against - only how far
    // the duck skids after releasing input. Updated per level by
    // LevelManager to match that level's LevelData::ground_friction
    // (defaults to 1 = today's stopping feel, unchanged).
    bn::fixed _ground_friction_scale = 1;
};