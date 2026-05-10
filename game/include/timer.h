#pragma once

#include <bn_sprite_text_generator.h>
#include <bn_string.h>
#include <bn_vector.h>
#include "bn_sprite_ptr.h"

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
    TimerHUD(bn::sprite_text_generator& text_gen, const Timer& timer);

    void update();
    void set_visible(bool visible);
    bool visible() const;

   private:
    void refresh();

    bn::sprite_text_generator& _text_gen;
    const Timer& _timer;
    bn::vector<bn::sprite_ptr, 16> _sprites;
    bool _visible = true;
};