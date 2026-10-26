#pragma once

#include "bn_fixed.h"

#include "locomotion.h"
#include "physics_body.h"
#include "sprite.h"

// -----------------------------------------------------------------------------
// PlayerDeathHandler
//
// Callback interface so the movement rules can trigger a death exactly at the
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
// Connects the movement rules (locomotion::Core, host-tested) to the game:
// reads the buttons once per frame, lets the core move the PhysicsBody with
// its collision probes, then plays the sounds, records the jump / dash for
// triggers (PlayerEvents) and turns the sprite the way the duck faces.
// -----------------------------------------------------------------------------
class PlayerLocomotion {
   public:
    PlayerLocomotion(
        PhysicsBody& body,
        Sprite& sprite,
        uint16_t block_mask,
        PlayerDeathHandler& death_handler);

    // One frame of movement.
    void update();

    bool on_ground() const { return _core.on_ground(); }

    // True while a dash is in progress (read by Player for animation).
    bool dashing() const { return _core.dashing(); }

    // See locomotion::Core. Called by LevelManager::load() for every level.
    void set_horizontal_bound(bn::fixed bound) {
        _core.set_horizontal_bound(bound);
    }
    void set_ground_friction(bn::fixed scale) {
        _core.set_ground_friction(scale);
    }
    void set_assist_mode(bool enabled) { _core.set_assist_mode(enabled); }
    void set_dash_enabled(bool enabled) { _core.set_dash_enabled(enabled); }
    void set_wall_jump_enabled(bool enabled) {
        _core.set_wall_jump_enabled(enabled);
    }

    // Cancels a running dash (death, level load).
    void reset_dash() { _core.reset_dash(); }

    // Emulator tests (replay.h): called with the buttons read from the
    // keypad every movement step; may record them or replace them.
    // nullptr (the default) = the keypad as it is.
    using InputHook = void (*)(InputState& input);
    void set_input_hook(InputHook hook) { _input_hook = hook; }

   private:
    // The collision probes and the death of the real body, for Core::step().
    struct World {
        PhysicsBody& body;
        uint16_t block_mask;
        PlayerDeathHandler& death_handler;

        bool probe_ground() { return body.probe_bottom(block_mask).any(); }
        bool probe_ceiling() { return body.probe_top(block_mask).any(); }
        bool probe_left() { return body.probe_left(block_mask).any(); }
        bool probe_right() { return body.probe_right(block_mask).any(); }
        void die() { death_handler.on_locomotion_death(); }
    };

    PhysicsBody& _body;
    Sprite& _sprite;
    uint16_t _block_mask;
    PlayerDeathHandler& _death_handler;
    InputHook _input_hook = nullptr;
    locomotion::Core _core;
};
