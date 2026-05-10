#include "timer.h"

void Timer::reset() {
    _counted_frames = 0;
}

void Timer::tick() {
    ++_counted_frames;
}

unsigned int Timer::counted_frames() const {
    return _counted_frames;
}

TimerHUD::TimerHUD(bn::sprite_text_generator& text_gen, const Timer& timer)
    : _text_gen(text_gen), _timer(timer) {
    // Initialize HUD
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
    if (!_visible) {
        _sprites.clear();
    } else {
        refresh();
    }
}

bool TimerHUD::visible() const {
    return _visible;
}

void TimerHUD::refresh() {
    _sprites.clear();

    const unsigned int minutes = _timer.counted_frames() / 3600;
    const unsigned int seconds = _timer.counted_frames() / 60 % 60;
    const unsigned int millis = _timer.counted_frames() * 5 / 3 % 100;

    bn::string<16> text;

    if (minutes < 10)
        text += "0";
    text += bn::to_string<8>(minutes);
    text += ":";

    if (seconds < 10)
        text += "0";
    text += bn::to_string<2>(seconds);
    text += ":";

    if (millis < 10)
        text += "0";
    text += bn::to_string<2>(millis);

    _text_gen.generate(Cfg::Timer::X, Cfg::Timer::Y, text, _sprites);
}