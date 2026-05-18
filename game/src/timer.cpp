#include "timer.h"

void Timer::reset() {
    _counted_frames = 0;
}

void Timer::tick() {
    ++_counted_frames;

    _centis = (_counted_frames * 5) / 3;

    if (_counted_frames >= 60) {
        _counted_frames = 0;
        _centis = 0;
        ++_seconds;

        if (_seconds >= 60) {
            _seconds = 0;
            ++_minutes;
        }
    }
}

unsigned int Timer::counted_frames() const {
    return _counted_frames;
}

unsigned int Timer::centis() const {
    return _centis;
}

unsigned int Timer::seconds() const {
    return _seconds;
}

unsigned int Timer::minutes() const {
    return _minutes;
}

void Timer::setAll(
    unsigned int centis,
    unsigned int seconds,
    unsigned int minutes) {
    _centis = centis;
    _seconds = seconds;
    _minutes = minutes;
}

TimerHUD::TimerHUD(const Timer& timer) : _timer(timer) {
    const auto& font_item = bn::sprite_items::common_fixed_8x16_font;
    const auto& font_tiles = font_item.tiles_item();

    // Cache numbers 0 - 9
    for (int i = 0; i <= 9; i++) {
        _cached_tiles.push_back(font_tiles.create_tiles(15 + i));
    }
    // Cache ":" symbol
    _cached_tiles.push_back(font_tiles.create_tiles(25));

    // Initialize text sprites
    for (int i = 0; i < 8; i++) {
        _sprites.push_back(font_item.create_sprite(
            Cfg::Timer::X + (i * 8), Cfg::Timer::Y, 15));
    }

    refresh();
}

void TimerHUD::update() {
    // Only update when timer visible
    if (_visible) {
        refresh();
    }
}

void TimerHUD::set_visible(bool visible) {
    if (_visible == visible) {
        return;
    }

    _visible = visible;

    for (auto& sprite : _sprites) {
        sprite.set_visible(_visible);
    }

    if (_visible) {
        refresh();
    }
}

bool TimerHUD::visible() const {
    return _visible;
}

void TimerHUD::refresh() {
    unsigned int m1 = 0;
    unsigned int m2 = _timer.minutes();
    while (m2 >= 10) {
        m2 -= 10;
        ++m1;
    }

    unsigned int s1 = 0;
    unsigned int s2 = _timer.seconds();
    while (s2 >= 10) {
        s2 -= 10;
        ++s1;
    }

    unsigned int c1 = 0;
    unsigned int c2 = _timer.centis();
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

void TimerHUD::set_digit(int index, int value) {
    if (_displayed_digits[index] != value) {
        _displayed_digits[index] = value;
        _sprites[index].set_tiles(_cached_tiles[value]);
    }
}
