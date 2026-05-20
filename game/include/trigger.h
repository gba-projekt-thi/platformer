#pragma once

#include "bn_fixed.h"

#include "cfg.h"
#include "physics_body.h"

// -----------------------------------------------------------------------------
// Trigger
//
// Invisible collision volume used to activate gameplay events.
//
// Typically linked to:
// - traps
// - moving platforms
// - scripted events
//
// Trigger state is intentionally simple and lightweight.
// -----------------------------------------------------------------------------
class Trigger : public PhysicsBody {
   public:
    Trigger(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height,
        bool default_on = false);

    bool is_triggered() const;

    void on_enter(uint16_t hit_layers, StaticBody* body) override;

    // Explicit state controls improve readability/debugging.
    void activate();
    void deactivate();

    // Restores original startup state.
    void reset();

   private:
    bool triggered;
    bool default_on;
};