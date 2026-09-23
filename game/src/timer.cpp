#include "timer.h"

namespace {

// Converts frame count into centiseconds.
//
// GBA runs at 60 FPS:
// 1 frame ~= 1.666 centiseconds
//
// Using a lookup table avoids runtime division.
constexpr uint8_t CENTI_TABLE[60] = {
    0,  1,  3,  5,  6,  8,  10, 11, 13, 15, 16, 18, 20, 21, 23,
    25, 26, 28, 30, 31, 33, 35, 36, 38, 40, 41, 43, 45, 46, 48,
    50, 51, 53, 55, 56, 58, 60, 61, 63, 65, 66, 68, 70, 71, 73,
    75, 76, 78, 80, 81, 83, 85, 86, 88, 90, 91, 93, 95, 96, 98};

struct Digits {
    uint8_t tens;
    uint8_t ones;
};

// Precomputed digit decomposition table.
//
// Avoids division/modulo during HUD rendering.
constexpr Digits DIGIT_TABLE[100] = {
    {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8},
    {0, 9}, {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7},
    {1, 8}, {1, 9}, {2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6},
    {2, 7}, {2, 8}, {2, 9}, {3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}, {3, 5},
    {3, 6}, {3, 7}, {3, 8}, {3, 9}, {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4},
    {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}, {5, 0}, {5, 1}, {5, 2}, {5, 3},
    {5, 4}, {5, 5}, {5, 6}, {5, 7}, {5, 8}, {5, 9}, {6, 0}, {6, 1}, {6, 2},
    {6, 3}, {6, 4}, {6, 5}, {6, 6}, {6, 7}, {6, 8}, {6, 9}, {7, 0}, {7, 1},
    {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 7}, {7, 8}, {7, 9}, {8, 0},
    {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5}, {8, 6}, {8, 7}, {8, 8}, {8, 9},
    {9, 0}, {9, 1}, {9, 2}, {9, 3}, {9, 4}, {9, 5}, {9, 6}, {9, 7}, {9, 8},
    {9, 9}};

constexpr int COLON_INDEX = 10;
}  // namespace

void Timer::reset() {
    _frame_counter = 0;
    _centis = 0;
    _seconds = 0;
    _minutes = 0;
}

void Timer::tick() {
    ++_frame_counter;

    // Advance seconds every 60 frames.
    if (_frame_counter >= 60) {
        _frame_counter = 0;

        ++_seconds;

        if (_seconds >= 60) {
            _seconds = 0;
            ++_minutes;
        }
    }

    // Lookup instead of division.
    _centis = CENTI_TABLE[_frame_counter];
}

uint16_t Timer::centis() const {
    return _centis;
}

uint16_t Timer::seconds() const {
    return _seconds;
}

uint16_t Timer::minutes() const {
    return _minutes;
}

void Timer::set_time(uint16_t centis, uint16_t seconds, uint16_t minutes) {
    _centis = centis;
    _seconds = seconds;
    _minutes = minutes;
}

TimerHUD::TimerHUD(const Timer& timer) : _timer(timer) {
    const auto& font_item = bn::sprite_items::common_fixed_8x16_font;

    const auto& font_tiles = font_item.tiles_item();

    // Cache digit tiles (0-9).
    for (int i = 0; i <= 9; ++i) {
        _cached_tiles.push_back(font_tiles.create_tiles(15 + i));
    }

    // Cache ':' tile.
    _cached_tiles.push_back(font_tiles.create_tiles(25));

    // Create HUD sprites.
    for (int i = 0; i < 8; ++i) {
        auto sprite =
            font_item.create_sprite(Cfg::Timer::X + (i * 8), Cfg::Timer::Y, 15);

        sprite.set_blending_enabled(true);

        _sprites.push_back(bn::move(sprite));
    }

    refresh();
}

void TimerHUD::update() {
    if (!_visible) {
        return;
    }

    refresh();
}

void TimerHUD::set_visible(bool visible) {
    if (_visible == visible) {
        return;
    }

    _visible = visible;

    for (auto& sprite : _sprites) {
        sprite.set_visible(_visible);
    }

    // Force refresh after becoming visible.
    if (_visible) {
        refresh();
    }
}

bool TimerHUD::visible() const {
    return _visible;
}

void TimerHUD::refresh() {
    const auto minute_digits = DIGIT_TABLE[_timer.minutes()];

    const auto second_digits = DIGIT_TABLE[_timer.seconds()];

    const auto centi_digits = DIGIT_TABLE[_timer.centis()];

    set_digit(0, minute_digits.tens);
    set_digit(1, minute_digits.ones);

    set_digit(2, COLON_INDEX);

    set_digit(3, second_digits.tens);
    set_digit(4, second_digits.ones);

    set_digit(5, COLON_INDEX);

    set_digit(6, centi_digits.tens);
    set_digit(7, centi_digits.ones);
}

void TimerHUD::set_digit(int index, int value) {
    // Avoid redundant VRAM tile swaps.
    if (_displayed_digits[index] == value) {
        return;
    }

    _displayed_digits[index] = value;

    _sprites[index].set_tiles(_cached_tiles[value]);
}