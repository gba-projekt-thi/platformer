#pragma once

#include "base_trap.h"
#include "bn_sprite_items_door32x32.h"
#include "cfg.h"

class Door : public PhysicsBody {
   public:
    Door(bn::fixed in_x = 0, bn::fixed in_y = 0);

    // Animate the door each frame.
    void update();
    // Mark the door as reached when the player collides with it.
    void on_enter(
        [[maybe_unused]] uint16_t hit_layers,
        [[maybe_unused]] StaticBody* body) override;
    // Returns whether the player has reached the door.
    bool reached() const;
    virtual ~Door();

   private:
    bool _reached = false;
    Sprite door_sprite;
    bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES> action;
};
