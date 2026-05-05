#pragma once

#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_item.h"
#include "bn_vector.h"
#include "bn_span.h"

#include "cfg.h"
#include "physics_body.h"
#include "player.h"
#include "sprite.h"

class BaseTrap : public PhysicsBody {
   public:
    // Create a basic trap that can kill the player on contact.
    BaseTrap(
        bn::fixed t_x,
        bn::fixed t_y,
        bn::fixed t_width,
        bn::fixed t_height,
        const bn::sprite_item& t_sprite,
        int t_sprite_waits,
        bn::span<const uint16_t> t_graphics_indexes,
        uint16_t t_block,
        bn::fixed t_max_vel = 0);

    // Animate the trap each frame.
    virtual void update() override;
    // Handle player collision by triggering death.
    virtual void on_enter([[maybe_unused]] uint16_t hit_layers, StaticBody* body) override;

   private:
    Sprite trap_sprite;
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>> action;
    bn::span<const uint16_t> graphics_indexes;

};
