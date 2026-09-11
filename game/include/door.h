#pragma once

#include "bn_sprite_animate_actions.h"

#include "cfg.h"
#include "physics_body.h"
#include "sprite.h"

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
    Sprite _door_sprite;
    bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES> _action;
};
