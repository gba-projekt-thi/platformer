#pragma once

#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_item.h"
#include "bn_sprite_items_door32x32.h"

#include "cfg.h"
#include "physics_body.h"
#include "sprite.h"

class Door : public PhysicsBody {
   public:
    // sprite_item defaults to the standard animated door32x32 asset.
    // Passing a different sprite item (e.g. a narrative "goal" marker)
    // together with its own graphics_indexes renders/animates that sprite
    // instead. An empty graphics_indexes span means static/non-animated -
    // same convention as TrapData's graphics_indexes (see BaseTrap).
    Door(
        bn::fixed in_x = 0,
        bn::fixed in_y = 0,
        const bn::sprite_item& sprite_item = bn::sprite_items::door32x32,
        bn::span<const uint16_t> graphics_indexes = bn::span<const uint16_t>(),
        int animation_wait = 8,
        bool flip_horizontal = false);

    // Animate the door each frame (no-op when constructed with an empty
    // graphics_indexes span).
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
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>> _action;
};