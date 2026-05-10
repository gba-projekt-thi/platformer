#pragma once

#include <bn_vector.h>
#include "bn_sprite_items_common_fixed_8x16_font.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"

#include "cfg.h"

class Timer {
   private:
    unsigned int _counted_frames = 0;

   public:
    void reset();
    void tick();
    unsigned int counted_frames() const;
};

class TimerHUD {
   public:
    TimerHUD(const Timer& timer);

    void update();
    void set_visible(bool visible);
    bool visible() const;

   private:
    void refresh();
    void set_digit(int index, int value);

    const Timer& _timer;

    bn::vector<bn::sprite_ptr, 16> _sprites;
    bn::vector<bn::sprite_tiles_ptr, 11> _cached_tiles;

    int _displayed_digits[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    bool _visible = true;
};