#pragma once

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"

#include "cfg.h"
#include "door_flee.h"
#include "level_structure.h"
#include "physics_body.h"
#include "sprite.h"

class Door : public PhysicsBody {
   public:
    // DoorData::sprite defaults to the standard animated door32x32 asset; a
    // different sprite item (e.g. a narrative "goal" marker) comes with its
    // own graphics indexes. An empty graphics span means static /
    // non-animated - same convention as TrapData's graphics_indexes.
    explicit Door(const DoorData& data);

    // Animates the door each frame (no-op without graphics indexes).
    void update();
    // A running-away door (DoorData::flee_path) takes its next step; call
    // once per frame with the duck's position.
    void flee_from(bn::fixed duck_x, bn::fixed duck_y);
    // Back to its origin (death, restart).
    void reset_flee();
    // A locked door (a boss still lives) is hidden and cannot be entered.
    void set_locked(bool locked);
    // Mark the door as reached when the player collides with it.
    void on_enter(
        [[maybe_unused]] uint16_t hit_layers,
        [[maybe_unused]] StaticBody* body) override;
    // Returns whether the player has reached the door.
    bool reached() const;
    virtual ~Door();

   private:
    void _place();

    bn::fixed _origin_x;
    bn::fixed _origin_y;
    door_flee::Flee<bn::fixed_point> _flee;
    bool _reached = false;
    Sprite _door_sprite;
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>> _action;
};
