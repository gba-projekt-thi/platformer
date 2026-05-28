#pragma once

#include <cstdint>

#include <bn_vector.h>
#include "bn_sprite_items_common_fixed_8x16_font.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"

#include "cfg.h"

// Lightweight timer optimized for GBA.
//
// Avoids expensive division/modulo during runtime.
// Uses lookup tables for centisecond conversion.
class Timer {
   public:
    void reset();

    // Call once per frame (60 FPS).
    void tick();

    [[nodiscard]] uint16_t centis() const;
    [[nodiscard]] uint16_t seconds() const;
    [[nodiscard]] uint16_t minutes() const;

    void set_time(uint16_t centis, uint16_t seconds, uint16_t minutes);

   private:
    // Counts frames within the current second.
    uint8_t _frame_counter = 0;

    uint8_t _centis = 0;
    uint8_t _seconds = 0;

    // Minutes can grow larger.
    uint16_t _minutes = 0;
};

class TimerHUD {
   public:
    explicit TimerHUD(const Timer& timer);

    // Updates visible digits only when values change.
    void update();

    void set_visible(bool visible);

    [[nodiscard]] bool visible() const;

   private:
    void refresh();

    // Changes a digit sprite only if needed.
    void set_digit(int index, int value);

   private:
    const Timer& _timer;

    // 8 sprites:
    // MM:SS:CC
    bn::vector<bn::sprite_ptr, 8> _sprites;

    // Cached tiles:
    // 0-9 + ':'
    bn::vector<bn::sprite_tiles_ptr, 11> _cached_tiles;

    // Tracks currently displayed digits
    // to avoid redundant VRAM tile swaps.
    int _displayed_digits[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

    bool _visible = true;
};