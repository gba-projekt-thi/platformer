#pragma once

#include "base_trap.h"
#include "bn_sprite_items_door32x32.h"

class Door : public PhysicsBody {
   public:
    static constexpr uint16_t LAYERS = 0x0002;
    static constexpr uint16_t MASK = Player::LAYERS;

    static constexpr int MAX_ANIMATION_FRAMES = 8;

    Door(bn::fixed in_x = 0, bn::fixed in_y = 0);

    // Animate the door each frame.
    void update();
    // Mark the door as reached when the player collides with it.
    void on_enter([[maybe_unused]] StaticBody& body) override;
    // Returns whether the player has reached the door.
    bool reached();

   private:
    bool _reached = 0;
    Sprite trap_sprite;
    bn::sprite_animate_action<MAX_ANIMATION_FRAMES> action;
};
