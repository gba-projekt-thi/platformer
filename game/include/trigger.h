#pragma once

#include <bn_fixed.h>
#include <bn_vector.h>

#include "cfg.h"
#include "physics_body.h"
#include "player.h"

class Trigger : public PhysicsBody {
   public:
    Trigger(
        bn::fixed t_start_x,
        bn::fixed t_start_y,
        bn::fixed t_width,
        bn::fixed t_height,
        bool t_default_on = false); //allows trap to be deactivatable instead of activatable
    // Returns whether this trigger has been activated.
    bool is_triggered() const;
    // Activate the trigger when the player enters its area.
    void on_enter(uint16_t hit_layers, StaticBody* body) override;
    // Reset the trigger for later reuse.
    void reset();

   private:
    bool triggered;
    bool default_on;
};