#pragma once

#include "cfg.h"
#include "physics_body.h"

// ----------------------------------------------------------------------------
// Trigger
//
// Invisible collision area used to activate traps.
//
// Trigger behavior:
// - Activates when player enters collision area
// - Can start enabled or disabled
// - Resettable on death/restart
// ----------------------------------------------------------------------------
class Trigger : public PhysicsBody {
   public:
    Trigger(
        bn::fixed t_x,
        bn::fixed t_y,
        bn::fixed t_width,
        bn::fixed t_height,
        bool t_default_on = false);

    bool is_triggered() const;

    void on_enter(uint16_t hit_layers, StaticBody* body) override;

    void reset();

   private:
    bool triggered;
    bool default_on;
};