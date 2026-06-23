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
class BaseTrap : public PhysicsBody {
   public:
    BaseTrap(
        bn::fixed t_x,
        bn::fixed t_y,
        bn::fixed t_width,
        bn::fixed t_height,
        bn::fixed t_offset_x,
        bn::fixed t_offset_y,
        const bn::sprite_item& t_sprite_item,
        int t_sprite_waits,
        bn::span<const uint16_t> t_graphics_indexes,
        uint16_t t_blocking_layers,
        bn::fixed t_max_vel = 0);

    virtual void update() override;

    // Kill the player on collision.
    virtual void on_enter(uint16_t hit_layers, StaticBody* body) override;

    virtual ~BaseTrap();

    // Reset trap state after player death or level restart.
    virtual void reset() {}

   protected:
    // Sprite wrapper synchronized through SpriteRegistry.
    Sprite trap_sprite;

   private:
    // Optional looping animation action.
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES> >
        _animation_action;
};