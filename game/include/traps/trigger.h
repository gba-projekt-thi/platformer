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
// - Jump / Dash action: the duck must jump / dash inside the zone instead of
//   merely entering it (see TriggerAction)
// - Timer: switches on by itself delay_frames after the level (re)starts
// - delay_frames > 0: traps and platforms follow the player's action only
//   after that many frames; a visible switch still flips at once
// - Follower (TriggerData::follows): ignores the player and is switched by
//   another trigger via follow() - LevelManager runs the chain
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
        int t_sprite_index_on = 1,
        unsigned t_delay_frames = 0,
        TriggerAction t_action = TriggerAction::Enter,
        bool t_follower = false);
    bool is_triggered() const;
    // Runs the delay / timer of the trigger state (see trigger_logic.h).
    void update() override;
    void on_enter(uint16_t hit_layers, StaticBody* body) override;
    void on_exit(uint16_t hit_layers, StaticBody* body) override;
    void reset() override;
    // Delayed trap chain: the source trigger's state (see TriggerChains).
    void follow(bool source_on);
    // Checkpoint respawn: puts the state back to a snapshot (not for Hold).
    void restore(bool triggered);

    [[nodiscard]] const char* name() const { return _name; }

   private:
    // Shows the tile matching the pressed state (no-op without a sprite).
    void _refresh_visual();

    TriggerZone _zone;
    const char* _name;

    // Optional switch visual; both tile sets are created up front so a state
    // change is just a tile swap (same idea as PlayerAnimator's tile cache).
    bn::optional<Sprite> _sprite;
    bn::optional<bn::sprite_tiles_ptr> _tiles_off;
    bn::optional<bn::sprite_tiles_ptr> _tiles_on;
};
