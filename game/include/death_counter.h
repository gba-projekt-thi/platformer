#pragma once

#include <bn_sprite_text_generator.h>
#include <bn_string.h>
#include <bn_vector.h>
#include "bn_sprite_ptr.h"

#include "cfg.h"

class DeathCounter {
   private:
    unsigned int _count = 0;

   public:
    // Called whenever the player dies to increment the death count.
    void on_player_death();
    // Returns the current death count.
    unsigned int count() const;
    void set_count(unsigned int count);
};

class DeathCounterHUD {
   public:
    DeathCounterHUD(
        bn::sprite_text_generator& text_gen,
        const DeathCounter& counter);
    // Update the HUD if the death count has changed.
    void update();
    // Control visibility of HUD sprites.
    void set_visible(bool visible);
    [[nodiscard]] bool visible() const;

   private:
    void refresh();

    bn::sprite_text_generator& _text_gen;
    const DeathCounter& _counter;
    bn::vector<bn::sprite_ptr, Cfg::DeathCounter::STRING_LEN> _sprites;
    uint32_t _last_count;
    bool _visible = true;
};
