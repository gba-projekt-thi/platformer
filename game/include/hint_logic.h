#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// hint
//
// Pure logic of the one-time level hint (LevelData::intro_hint): when it is
// shown and how long it stays. Split out of LevelScene so it has no Butano
// dependency and can be unit-tested on the host (see
// tests/host/test_hint_logic.cpp).
//
// A hint is shown while its level has never been cleared: no personal best
// yet (GameState::best_time_frames[level] == 0) AND progression has not moved
// past it (GameState::furthest_level <= level). Either one alone is not
// enough: an Assist Mode clear records no best time but still advances
// furthest_level, and furthest_level is reset by a finished run while best
// times survive. So it needs no save field of its own: it disappears for good
// with the first clear, which also proves the player has understood it.
// -----------------------------------------------------------------------------

namespace hint {

// Longest hint text in characters. The variable-width 8x16 font is at most
// 8px per glyph, so 28 characters (224px) always fit the 240px screen.
// tools/Check_level_hints.py enforces this on the level data.
inline constexpr int MAX_CHARS = 28;

inline auto should_show(
    const char* text,
    uint32_t best_time_frames,
    int furthest_level,
    unsigned level_index) -> bool {
    const bool cleared =
        best_time_frames != 0 || furthest_level > static_cast<int>(level_index);
    return text != nullptr && text[0] != '\0' && !cleared;
}

// The text the scene hands on to the display: the level's hint while it is
// still to be shown, nullptr otherwise.
inline auto text_to_show(
    const char* text,
    uint32_t best_time_frames,
    int furthest_level,
    unsigned level_index) -> const char* {
    return should_show(text, best_time_frames, furthest_level, level_index)
               ? text
               : nullptr;
}

// Whether the hint's sprites exist this frame. While the game is paused they
// are released: Butano counts hidden sprites too, and the pause overlay needs
// up to ~57 glyph sprites (see tools/Check_sprite_budget.py), so a hint that
// merely hid would still take its slots.
inline auto should_draw(bool timer_visible, bool paused) -> bool {
    return timer_visible && !paused;
}

// When the countdown starts. A hint of a boss level is about the arena, which
// comes at the end of the level: it waits until the fight has begun (the duck
// reached the arena) instead of running out unseen at the level start.
inline auto should_start(bool waits_for_arena, bool arena_reached) -> bool {
    return !waits_for_arena || arena_reached;
}

// Frame countdown for a visible hint. Not started = not visible.
class Timer {
   public:
    explicit Timer(int frames) : _frames(frames) {}

    void start() { _remaining = _frames; }
    void dismiss() { _remaining = 0; }
    void tick() {
        if (_remaining > 0) {
            --_remaining;
        }
    }

    [[nodiscard]] auto visible() const -> bool { return _remaining > 0; }

   private:
    int _frames;
    int _remaining = 0;
};

// What the scene does with the hint's sprites this frame.
enum class Sprites : uint8_t { Keep, Draw, Release, Finish };

// The life of one hint: shown (or waiting for the arena), counting down only
// while the game runs, dismissed by B in play, its sprites released while
// paused and drawn again afterwards, finished when the countdown is over.
// LevelScene::_update_hint() does what update() says.
class Display {
   public:
    explicit Display(int frames) : _timer(frames) {}

    // The hint should be shown on this level. Returns whether to draw now
    // (a boss level's hint waits for the arena).
    auto show(bool waits_for_arena) -> bool {
        _active = true;
        _waits = waits_for_arena;
        if (!_waits) {
            _timer.start();
        }
        return !_waits;
    }

    [[nodiscard]] auto active() const -> bool { return _active; }

    // Once per frame, `sprites_alive`: the hint's sprites exist right now.
    auto
    update(bool paused, bool arena_reached, bool b_pressed, bool sprites_alive)
        -> Sprites {
        if (!_active) {
            return Sprites::Keep;
        }
        if (_waits && should_start(true, arena_reached)) {
            _waits = false;
            _timer.start();
        }
        if (_waits) {
            return Sprites::Keep;
        }
        // The countdown only runs while the game does, and B only dismisses
        // the hint in play (in the pause menu B means 'back').
        if (!paused) {
            if (b_pressed) {
                _timer.dismiss();
            }
            _timer.tick();
        }
        if (!_timer.visible()) {
            _active = false;
            return Sprites::Finish;
        }
        if (should_draw(_timer.visible(), paused)) {
            return sprites_alive ? Sprites::Keep : Sprites::Draw;
        }
        return Sprites::Release;
    }

   private:
    Timer _timer;
    bool _active = false;
    bool _waits = false;
};

// One frame of the scene around the level: Start toggles the pause, and the
// pause overlay allocates its ~57 sprites inside the level update, so the
// hint's sprites go first.
template <typename Host>
auto step_level(Host& h) {
    if (h.start_pressed()) {
        h.release_hint_sprites();
    }
    return h.update_level();
}

}  // namespace hint
