#pragma once

#include <bn_fixed.h>
#include <bn_sprite_animate_actions.h>
#include <bn_sprite_item.h>
#include <bn_vector.h>

#include "physics_body.h"
#include "player.h"
#include "sprite.h"

class BaseTrap : public PhysicsBody {
   public:
    static constexpr uint16_t LAYERS = 0x0002;
    static constexpr uint16_t MASK = Player::LAYERS;

    static constexpr int MAX_ANIMATION_FRAMES = 8;

    BaseTrap(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed t_width,
        bn::fixed t_height,
        const bn::sprite_item& t_sprite,
        uint16_t t_block);
    void update() override;
    void on_enter(StaticBody& body) override;

   private:
    Sprite trap_sprite;
    bn::sprite_animate_action<MAX_ANIMATION_FRAMES> action;
};
