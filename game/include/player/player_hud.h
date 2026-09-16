#pragma once

#include "bn_sprite_text_generator.h"
#include "common_variable_8x16_sprite_font.h"

#include "death_counter.h"
#include "timer.h"

// -----------------------------------------------------------------------------
// PlayerHud
//
// Bundles the player's on-screen HUD elements (death counter + timer).
// Owns no gameplay logic - just forwards updates/visibility to its two
// sub-widgets and exposes the small read/write surface Player needs.
// Extracted out of Player as the first step of the Player decomposition.
// -----------------------------------------------------------------------------
class PlayerHud {
   public:
    PlayerHud();

    // Advances the timer by one frame, handles the SELECT toggle, and
    // refreshes the HUD if visible. Call once per frame from Player::update().
    void tick();

    // Shows/hides both HUD elements (used when entering menus/title screens).
    void set_visible(bool visible);

    unsigned int deaths() const;
    void set_deaths(unsigned int deaths);
    void on_player_death();

    Timer& timer();

   private:
    DeathCounter _death_counter;
    bn::sprite_text_generator _death_counter_text_gen;
    DeathCounterHUD _death_counter_hud;

    Timer _timer;
    TimerHUD _timer_hud;
};