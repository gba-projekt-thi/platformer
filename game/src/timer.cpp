#include "timer.h"

void Timer::reset() {
    counted_frames = 0;
}

void Timer::tick() {
    ++counted_frames;

    centis = (counted_frames * 5) / 3;

    if (counted_frames >= 60) {
        counted_frames = 0;
        centis = 0;
        ++seconds;

        if (seconds >= 60) {
            seconds = 0;
            ++minutes;
        }
    }
}

unsigned int Timer::counted_frames_value() const {
    return counted_frames;
}

unsigned int Timer::centis_value() const {
    return centis;
}

unsigned int Timer::seconds_value() const {
    return seconds;
}

unsigned int Timer::minutes_value() const {
    return minutes;
}

TimerHud::TimerHud(const Timer& timer) : game_timer(timer) {
    const auto& font_item = bn::sprite_items::common_fixed_8x16_font;

    const auto& font_tiles = font_item.tiles_item();

    // Cache numbers 0 - 9
    for (int i = 0; i <= 9; i++) {
        cached_tiles.push_back(font_tiles.create_tiles(15 + i));
    }

    // Cache ":" symbol
    cached_tiles.push_back(font_tiles.create_tiles(25));

    // Initialize text sprites
    for (int i = 0; i < 8; i++) {
        sprites.push_back(font_item.create_sprite(
            Cfg::Timer::X + (i * 8), Cfg::Timer::Y, 15));
    }

    refresh();
}

void TimerHud::update() {
    // Only update when timer visible
    if (visible_flag) {
        refresh();
    }
}

void TimerHud::set_visible(bool visible) {
    if (visible_flag == visible) {
        return;
    }

    visible_flag = visible;

    for (auto& sprite : sprites) {
        sprite.set_visible(visible_flag);
    }

    if (visible_flag) {
        refresh();
    }
}

bool TimerHud::visible() const {
    return visible_flag;
}

void TimerHud::refresh() {
    unsigned int m1 = 0;
    unsigned int m2 = game_timer.minutes_value();

    while (m2 >= 10) {
        m2 -= 10;
        ++m1;
    }

    unsigned int s1 = 0;
    unsigned int s2 = game_timer.seconds_value();

    while (s2 >= 10) {
        s2 -= 10;
        ++s1;
    }

    unsigned int c1 = 0;
    unsigned int c2 = game_timer.centis_value();

    while (c2 >= 10) {
        c2 -= 10;
        ++c1;
    }

    int colon = 10;  // Index for ":"

    set_digit(0, m1);
    set_digit(1, m2);
    set_digit(2, colon);
    set_digit(3, s1);
    set_digit(4, s2);
    set_digit(5, colon);
    set_digit(6, c1);
    set_digit(7, c2);
}

void TimerHud::set_digit(int index, int value) {
    if (displayed_digits[index] != value) {
        displayed_digits[index] = value;

        sprites[index].set_tiles(cached_tiles[value]);
    }
}