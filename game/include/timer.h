#pragma once

#include "bn_sprite_items_common_fixed_8x16_font.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_vector.h"

#include "cfg.h"

class Timer {
   private:
    unsigned int counted_frames = 0;
    unsigned int centis = 0;
    unsigned int seconds = 0;
    unsigned int minutes = 0;

   public:
    void reset();
    void tick();
    unsigned int counted_frames_value() const;
    unsigned int centis_value() const;
    unsigned int seconds_value() const;
    unsigned int minutes_value() const;
};

class TimerHud {
   public:
    TimerHud(const Timer& timer);

    void update();
    void set_visible(bool visible);
    bool visible() const;

   private:
    void refresh();
    void set_digit(int index, int value);

    const Timer& timer;

    bn::vector<bn::sprite_ptr, 16> sprites;
    bn::vector<bn::sprite_tiles_ptr, 11> cached_tiles;

    int displayed_digits[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    bool visible_flag = true;
};