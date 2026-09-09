#pragma once

#include "bn_fixed.h"
#include "bn_log.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_ente.h"

#include "cfg.h"
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

    // PlayerDeathHandler
    void on_locomotion_death() override { death(); }
};