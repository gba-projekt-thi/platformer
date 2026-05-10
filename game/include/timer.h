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

    const Timer& _timer;

    bn::vector<bn::sprite_ptr, 16> _sprites;
    bn::vector<bn::sprite_tiles_ptr, 11> _cached_tiles;

    bool _visible = true;
};