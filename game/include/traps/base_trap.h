#pragma once

#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_item.h"

#include "cfg.h"
#include "i_resettable.h"
#include "level_structure.h"
#include "physics_body.h"
#include "sprite.h"

// ----------------------------------------------------------------------------
// BaseTrap
//
// Base class for all harmful trap entities.
//
// Responsibilities:
// - Collision handling against the player
// - Optional sprite animation
// - Sprite ownership/registration
//
// Derived classes:
// - MovingTrap
// - PathTrap
// ----------------------------------------------------------------------------
class BaseTrap : public PhysicsBody, public IResettable {
   public:
    // A trap at the position, size and look of `data` (the fields every
    // TrapType shares). max_vel caps a trap moved by velocity (MovingTrap);
    // traps that set their position directly pass
    // Cfg::Physics::NO_MAX_VELOCITY.
    explicit BaseTrap(const TrapData& data, bn::fixed max_vel = 0);

    virtual void update() override;

    // Kill the player on collision.
    virtual void on_enter(uint16_t hit_layers, StaticBody* body) override;

    virtual ~BaseTrap();

    // Reset trap state after player death or level restart.
    void reset() override {}

    // Level Devil (TrapData::from_death / until_death): false takes the trap
    // out of this attempt - no collision either way, no picture - true puts
    // it back. LevelManager calls it after reset(), only for traps with a
    // death window; never for a BOSS (its fight shows and hides it itself).
    void set_present(bool present);

    // Draw order of the trap's sprite (see TrapData::sprite_z_order).
    void set_sprite_z_order(int z_order) {
        _trap_sprite.sprite().set_z_order(z_order);
    }

   protected:
    // Same, but starting at (x, y) instead of (data.x, data.y).
    BaseTrap(const TrapData& data, bn::fixed x, bn::fixed y, bn::fixed max_vel);

    // Sprite wrapper synchronized through SpriteRegistry.
    Sprite _trap_sprite;

   private:
    // Optional looping animation action.
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES> >
        _animation_action;
};