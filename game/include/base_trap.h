#pragma once

#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_item.h"

#include "cfg.h"
#include "physics_body.h"
#include "player.h"
#include "sprite.h"

// -----------------------------------------------------------------------------
// BaseTrap
//
// Base damaging trap entity.
//
// Responsibilities:
// - Collision with player
// - Optional sprite animation
// - Sprite ownership
//
// Derived classes:
// - MovingTrap
// - PathTrap
// -----------------------------------------------------------------------------
class BaseTrap : public PhysicsBody {
   public:
    BaseTrap(
        bn::fixed x,
        bn::fixed y,
        bn::fixed width,
        bn::fixed height,
        const bn::sprite_item& sprite_item,
        int sprite_waits,
        bn::span<const uint16_t> graphics_indexes,
        uint16_t blocking_layers,
        bn::fixed max_vel = 0);

    virtual void update() override;

    virtual void on_enter(uint16_t hit_layers, StaticBody* body) override;

    virtual ~BaseTrap();

   protected:
    Sprite trap_sprite;

   private:
    // Optional looping animation.
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES> >
        _animation_action;

    // Animation frame indexes.
    // Could be removed later to reduce per-instance memory.
    bn::span<const uint16_t> _graphics_indexes;
};