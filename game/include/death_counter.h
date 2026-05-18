#pragma once

#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cfg.h"

class DeathCounter {
   private:
    unsigned int count_value = 0;

   public:
    // Called whenever the player dies to increment the death count.
    void on_player_death();
    // Returns the current death count.
    unsigned int count() const;
};

class DeathCounterHUD {
   public:
    DeathCounterHUD(
        bn::sprite_text_generator& text_gen,
        const DeathCounter& counter);
    // Update the HUD if the death count has changed.
    void update();

   private:
    void refresh();

    bn::sprite_text_generator& text_gen;
    const DeathCounter& counter;
    bn::vector<bn::sprite_ptr, Cfg::DeathCounter::STRING_LEN> sprites;
    uint32_t last_count;
};