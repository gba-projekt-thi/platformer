#pragma once

#include "bn_optional.h"
#include "bn_sprite_item.h"
#include "bn_sprite_tiles_ptr.h"

#include "i_resettable.h"
#include "physics_body.h"
#include "sprite.h"
#include "trigger_logic.h"

// ----------------------------------------------------------------------------
// Trigger
//
// Collision area used to activate traps and to open/close switched platforms
// (PlatformData::trigger_name).
//
// Trigger behavior (see TriggerMode in trigger_logic.h):
// - Latch (default): stays on after the player enters, until reset
// - Toggle: every entry flips it (a switch)
// - Hold: on only while the player is inside (a pressure plate)
// - Can start enabled or disabled
// - Resettable on death/restart: LevelManager resets triggers directly, so a
//   trigger driving only platforms resets too
// - Invisible by default; optionally draws a two-state sprite (off/on)
// ----------------------------------------------------------------------------
class Trigger : public PhysicsBody, public IResettable {
   public:
    Trigger(
        bn::fixed t_x,
        bn::fixed t_y,
        bn::fixed t_width,
        bn::fixed t_height,
        bool t_default_on = false,
        const char* t_name = nullptr,
        TriggerMode t_mode = TriggerMode::Latch,
        const bn::sprite_item* t_sprite = nullptr,
        int t_sprite_index_off = 0,
        int t_sprite_index_on = 1);
    bool is_triggered() const;
    void on_enter(uint16_t hit_layers, StaticBody* body) override;
    void on_exit(uint16_t hit_layers, StaticBody* body) override;
    void reset() override;

    [[nodiscard]] const char* name() const { return _name; }

   private:
    // Shows the tile matching the current state (no-op without a sprite).
    void _refresh_visual();

    TriggerState _state;
    const char* _name;

    // Optional switch visual; both tile sets are created up front so a state
    // change is just a tile swap (same idea as PlayerAnimator's tile cache).
    bn::optional<Sprite> _sprite;
    bn::optional<bn::sprite_tiles_ptr> _tiles_off;
    bn::optional<bn::sprite_tiles_ptr> _tiles_on;
};
