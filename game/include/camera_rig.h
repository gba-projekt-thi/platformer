#pragma once

#include "cfg_camera.h"

// -----------------------------------------------------------------------------
// CameraRig
//
// Decides where the camera aims each frame: horizontally a dead zone around the
// duck (the camera moves exactly as far as the duck once it leaves it, so the
// duck never wobbles on screen) plus an eased look-ahead in the direction it
// runs; vertically a smoothed follow with an optional look up / down.
// Split out of LevelManager so it has no Butano dependency and can be
// unit-tested on the host (see tests/host/test_camera_rig.cpp).
//
// The class is a template over the number type: bn::fixed on the device, a
// plain float/double (or an integer fixed-point stand-in) in the tests. It only
// needs +, -, * int, / int and comparisons. The rig knows nothing about the
// world: it returns where the camera WANTS to be and Camera::follow() clamps
// that to the world; set_bounds() keeps its own aim inside the same limits, so
// a duck that turns around at a world edge is followed at once instead of the
// aim having to travel back from beyond the edge first.
//
// Per-frame protocol (LevelManager::_update_camera()):
//   level load / respawn:  snap(x, y)
//   every frame:           update(duck x, duck y, duck vel_x, look)
//                          Camera::follow(x(), y())
// -----------------------------------------------------------------------------

namespace camera_rig {

// How far the camera centre may travel from the world centre without showing
// anything outside the world: half the world minus half the screen, never
// negative (a world that is not larger than the screen cannot scroll).
template <typename T>
auto max_offset(T world_size, int screen_size) -> T {
    const T value = world_size / 2 - T(screen_size / 2);
    return value < T(0) ? T(0) : value;
}

// The D-Pad as the rig's `look` argument: -1 up, +1 down, 0 neither (or both).
inline auto look(bool up_held, bool down_held) -> int {
    return (down_held ? 1 : 0) - (up_held ? 1 : 0);
}

}  // namespace camera_rig

template <typename T>
class CameraRig {
   public:
    // Limits of the aim on each axis, symmetrical around the world centre
    // (see camera_rig::max_offset()). The default is "unlimited".
    void set_bounds(T max_x, T max_y) {
        _max_x = max_x;
        _max_y = max_y;
        _bounded = true;
    }

    // Level load: the limits of the new level first, then the cut to the duck,
    // so the cut is already inside them.
    void place(T max_x, T max_y, T x, T y) {
        set_bounds(max_x, max_y);
        snap(x, y);
    }

    // Cuts to a position at once: level load, respawn. Forgets the direction
    // and any look so nothing eases in from the old position.
    void snap(T x, T y) {
        _x = _bounded ? _clamp(x, _max_x) : x;
        _y = _bounded ? _clamp(y, _max_y) : y;
        _focus_x = _x;
        _look_x = T(0);
        _look_y = T(0);
        _dir = 0;
        _run_dir = 0;
        _run_frames = 0;
        _look_held = 0;
        _look_hold_frames = 0;
    }

    // One frame. (target_x, target_y) is the duck, vel_x its horizontal
    // velocity in px/frame, `look` the D-Pad: -1 up, +1 down, 0 neither.
    void update(T target_x, T target_y, T vel_x, int look) {
        namespace C = Cfg::CameraFollow;

        // Direction of travel: only a run of LOOKAHEAD_HOLD_FRAMES counts, so
        // a tap does not swing the view; a duck that stands (or creeps) keeps
        // the last direction.
        const int run_dir = vel_x >= T(C::LOOKAHEAD_MIN_SPEED)    ? 1
                            : vel_x <= T(-C::LOOKAHEAD_MIN_SPEED) ? -1
                                                                  : 0;
        _run_frames = run_dir != 0 && run_dir == _run_dir ? _run_frames + 1
                      : run_dir != 0                      ? 1
                                                          : 0;
        _run_dir = run_dir;
        if (_run_dir != 0 && _run_frames >= C::LOOKAHEAD_HOLD_FRAMES) {
            _dir = _run_dir;
        }
        const T want_look_x = T(_dir * C::LOOKAHEAD_PX);
        _look_x +=
            (want_look_x - _look_x) * C::LOOKAHEAD_NUM / C::LOOKAHEAD_DEN;

        // Look up / down only starts after the D-Pad was held for a while, so
        // a brief tap (turning the duck to face away) does not move the view.
        if (look == 0) {
            _look_held = 0;
            _look_hold_frames = 0;
        } else if (look == _look_held) {
            ++_look_hold_frames;
        } else {
            _look_held = look;
            _look_hold_frames = 1;
        }
        const T want_look_y = _look_hold_frames >= C::LOOK_Y_HOLD_FRAMES
                                  ? T(_look_held * C::LOOK_Y_PX)
                                  : T(0);
        _look_y += (want_look_y - _look_y) * C::LOOK_Y_NUM / C::LOOK_Y_DEN;

        // Dead zone: the focus only moves once the duck is DEADZONE_X away.
        const T dead_zone = T(C::DEADZONE_X);
        if (target_x > _focus_x + dead_zone) {
            _focus_x = target_x - dead_zone;
        } else if (target_x < _focus_x - dead_zone) {
            _focus_x = target_x + dead_zone;
        }
        if (_bounded) {
            _focus_x = _clamp(_focus_x, _max_x);
        }

        T aim_x = _focus_x + _look_x;
        T aim_y = target_y + _look_y;
        if (_bounded) {
            aim_x = _clamp(aim_x, _max_x);
            aim_y = _clamp(aim_y, _max_y);
        }
        _x = aim_x;
        _y += (aim_y - _y) * C::FOLLOW_Y_NUM / C::FOLLOW_Y_DEN;
    }

    [[nodiscard]] auto x() const -> T { return _x; }
    [[nodiscard]] auto y() const -> T { return _y; }
    [[nodiscard]] auto look_x() const -> T { return _look_x; }
    [[nodiscard]] auto look_y() const -> T { return _look_y; }

   private:
    static auto _clamp(T value, T limit) -> T {
        if (value < -limit) {
            return -limit;
        }
        if (value > limit) {
            return limit;
        }
        return value;
    }

    T _x = T(0);
    T _y = T(0);
    T _focus_x = T(0);
    T _look_x = T(0);
    T _look_y = T(0);
    T _max_x = T(0);
    T _max_y = T(0);
    bool _bounded = false;
    int _dir = 0;
    int _run_dir = 0;
    int _run_frames = 0;
    int _look_held = 0;
    int _look_hold_frames = 0;
};

namespace camera_rig {

// One frame from the duck and the D-Pad (LevelManager::_update_camera()):
// the run speed feeds the look-ahead, up / down the look.
template <typename T>
void aim(
    CameraRig<T>& rig,
    T duck_x,
    T duck_y,
    T duck_vel_x,
    bool up_held,
    bool down_held) {
    rig.update(duck_x, duck_y, duck_vel_x, look(up_held, down_held));
}

}  // namespace camera_rig
