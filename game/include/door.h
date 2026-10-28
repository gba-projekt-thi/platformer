#pragma once

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"

#include "cfg.h"
#include "clear_logic.h"
#include "door_flee.h"
#include "door_logic.h"
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
    // The door that moves between attempts (DoorData::attempt_positions):
    // its origin for this many deaths in the level (door_logic.h), the flee
    // path starting over from there.
    void place_for_attempt(unsigned deaths);
    // A trapped door that killed the duck (door_logic.h) can be touched again.
    void clear_reached();
    // A locked door (a boss still lives) stands there faded to grey and cannot
    // be entered.
    void set_locked(bool locked);
    // A door that has just been unlocked flashes white and fades back (called
    // right after set_locked(false)).
    void unlock_glow();
    // The level-clear moment: the door blinks white while lit.
    void set_flash(bool lit);
    // Mark the door as reached when the player collides with it.
    void on_enter(
        [[maybe_unused]] uint16_t hit_layers,
        [[maybe_unused]] StaticBody* body) override;
    // Returns whether the player has reached the door.
    bool reached() const;
    virtual ~Door();

   private:
    void _place();
    void _refresh_tint();

    bn::fixed _x;
    bn::fixed _y;
    bn::span<const bn::fixed_point> _attempt_positions;
    unsigned _attempt_every;
    bn::fixed _origin_x;
    bn::fixed _origin_y;
    door_flee::Flee<bn::fixed_point> _flee;
    bool _reached = false;
    clear_logic::Glow _glow;
    bool _locked = false;
    bool _flash = false;
    Sprite _door_sprite;
    bn::optional<bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>> _action;
};
