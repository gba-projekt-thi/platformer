#pragma once

#include "bn_fixed.h"

#include "bn_optional.h"

#include "cfg.h"
#include "dash_trail.h"
#include "physics_body.h"
#include "player_animator.h"
#include "player_hud.h"
#include "player_locomotion.h"
#include "player_state_machine.h"
#include "sprite.h"

class Player : public PhysicsBody, private PlayerDeathHandler {
   public:
    // Mask layer
    static constexpr uint16_t MASK = Cfg::Layer::TRAP | Cfg::Layer::DOOR;

    // Collision layers for the player and other objects.
    static constexpr uint16_t BLOCK =
        0xFFFF & ~Cfg::Layer::TRAP & ~Cfg::Layer::TRIGGER & ~Cfg::Layer::DOOR;

    Player(
        bn::fixed in_start_x,
        bn::fixed in_start_y,
        bn::fixed in_width,
        bn::fixed in_height);

    // Main per-frame update for player motion and animation.
    void update() override;

    // Handle player death and respawn.
    void death();

    // Set the current respawn location.
    void set_spawn_point(bn::fixed in_x, bn::fixed in_y);
    // teleport the player immediately to the given coordinate.
    void teleport_to(bn::fixed in_x, bn::fixed in_y);
    // Control player visibility while showing menu/UI.
    void set_visible(bool visible);
    [[nodiscard]] bool visible() const;
    // Control HUD visibility for title screens.
    void set_hud_visible(bool visible);
    // Updates the horizontal bounce boundary for the current level.
    void set_horizontal_bound(bn::fixed bound);
    // Per-level ground braking multiplier (1 = normal, <1 = slippery). See
    // PlayerLocomotion::set_ground_friction().
    void set_ground_friction(bn::fixed scale);
    // Assist Mode: widens coyote time / jump buffer. See
    // PlayerLocomotion::set_assist_mode().
    void set_assist_mode(bool enabled);
    // Dash (B): enabled per level by LevelManager::load().
    void set_dash_enabled(bool enabled);
    // Wall slide / wall jump: enabled per level by LevelManager::load().
    void set_wall_jump_enabled(bool enabled);
    // Pogo strike (R): enabled per level by LevelManager::load().
    void set_pogo_enabled(bool enabled);
    // Emulator tests: see PlayerLocomotion::set_input_hook().
    void set_input_hook(PlayerLocomotion::InputHook hook) {
        _locomotion.set_input_hook(hook);
    }
    // Removes the dash's afterimage ghosts at once (level complete, menus,
    // death) so none stays frozen on screen.
    void clear_dash_trail();
    // Death feedback (see death_fx.h). begin_death_fx() is called by
    // LevelManager right after a death: the duck is already back at its
    // respawn point, so it is hidden and a copy of its last frame stays where
    // it died while the world is frozen. end_death_fx() ends that, shows the
    // duck again and makes it flicker for a moment (visual only, no
    // invulnerability); cancel_death_fx() does the same without the flicker
    // (level load / unload).
    void begin_death_fx();
    void end_death_fx();
    void cancel_death_fx();
    // Get & Set the count of player deaths.
    unsigned int get_deaths() const;
    void set_deaths(unsigned int deaths);
    Timer& get_timer();

   private:
    // Sprite
    Sprite _player_sprite;

    // Physics/input, extracted into its own component.
    PlayerLocomotion _locomotion;

    // Walk/jump animation, extracted into its own component.
    PlayerAnimator _animator;

    // HUD (death counter + timer), extracted into its own component.
    PlayerHud _hud;

    // Coarse movement state (Idle/Run/Jump/Fall), extracted into its own
    // component.
    PlayerStateMachine _state_machine;

    // Respawn position
    bn::fixed _restart_x;
    bn::fixed _restart_y;

    // Dash afterimages: DashTrail decides when/where ghosts spawn and how
    // they fade (pure, host-tested); one Sprite per slot mirrors it.
    void _update_dash_trail();
    void _spawn_ghost(int slot);
    void _refresh_ghosts();
    DashTrail _trail;
    bn::optional<Sprite> _ghosts[DashTrail::SLOTS];
    bool _was_dashing = false;

    // Death feedback: where the duck died, the copy of it left there while
    // the world is frozen, and the flicker countdown after it comes back.
    void _update_respawn_blink();
    bn::fixed _death_x = 0;
    bn::fixed _death_y = 0;
    bn::optional<Sprite> _death_ghost;
    bool _death_fx_active = false;
    int _respawn_blink = 0;

    // The pogo strike's slash, shown below the duck while it reaches.
    void _update_pogo_slash();
    bn::optional<Sprite> _pogo_slash;

    // PlayerDeathHandler
    void on_locomotion_death() override { death(); }
};