#pragma once

// -----------------------------------------------------------------------------
// DashController
//
// Pure decision logic of the dash ability, split out of PlayerLocomotion so
// it has no Butano/GBA dependency and can be unit-tested on the host (see
// tests/host/test_dash_controller.cpp). PlayerLocomotion gathers this frame's
// inputs and environment flags into a DashInput, calls update(), and applies
// the returned DashStep (velocity, facing, sound).
//
// Lifecycle of one dash, counted in update() calls:
//   - the call that starts it already moves (moving = true, started = true),
//   - it moves for exactly `duration_frames` calls in total,
//   - the next call reports ended = true and moving = false, so normal
//     movement resumes on that same frame,
//   - a cooldown of `cooldown_frames` then has to elapse before the next
//     dash can start.
// A dash also ends early when a wall blocks it (after its first frame), when
// the world's bounce edge in its direction is reached, or when a jump is
// buffered while on the ground (dash-jump).
//
// The air-dash charge is spent when a dash starts and only refilled by
// land() - which the caller must call on REAL ground contact, never during
// coyote time, or dashing off a ledge would refill it at once.
// -----------------------------------------------------------------------------

struct DashInput {
    bool dash_pressed = false;  // B pressed this frame
    bool left_held = false;
    bool right_held = false;
    bool facing_left = false;    // fallback direction when nothing is held
    bool jump_buffered = false;  // jump buffer timer > 0
    bool on_ground = false;      // resolved ground state (incl. coyote time)
    bool vel_x_zero = false;     // a wall zeroed the horizontal velocity
    bool at_left_edge = false;   // at/past the world's left bounce edge
    bool at_right_edge = false;  // at/past the world's right bounce edge
};

struct DashStep {
    bool started = false;  // a dash started in this call
    bool ended = false;    // a dash ended in this call
    bool moving = false;   // the dash owns this frame: move at +-SPEED
    int dir = 1;           // -1 = left, +1 = right
};

class DashController {
   public:
    DashController(int duration_frames, int cooldown_frames)
        : _duration(duration_frames), _cooldown(cooldown_frames) {}

    // Enables/disables the ability. Always cancels a running dash and
    // refills the air dash.
    void set_enabled(bool enabled) {
        _enabled = enabled;
        reset();
    }

    // Cancels a running dash, clears the cooldown and refills the air dash.
    void reset() {
        _dashing = false;
        _timer = 0;
        _cooldown_timer = 0;
        _available = true;
    }

    // Real ground contact: refills the air dash.
    void land() { _available = true; }

    [[nodiscard]] auto dashing() const -> bool { return _dashing; }
    [[nodiscard]] auto available() const -> bool { return _available; }
    [[nodiscard]] auto cooldown_remaining() const -> int {
        return _cooldown_timer;
    }

    // Advances the controller by one frame.
    auto update(const DashInput& in) -> DashStep {
        DashStep step;
        step.dir = _dir;

        if (!_dashing) {
            if (_cooldown_timer > 0) {
                --_cooldown_timer;
            }
            if (!_enabled || !in.dash_pressed || !_available ||
                _cooldown_timer > 0) {
                return step;
            }

            if (in.left_held) {
                _dir = -1;
            } else if (in.right_held) {
                _dir = 1;
            } else {
                _dir = in.facing_left ? -1 : 1;
            }
            _dashing = true;
            _timer = _duration;
            _available = false;
            step.started = true;
            step.dir = _dir;
        }

        const bool first_frame = (_timer == _duration);
        const bool blocked = !first_frame && in.vel_x_zero;
        const bool at_edge =
            (_dir > 0 && in.at_right_edge) || (_dir < 0 && in.at_left_edge);
        const bool jump_cancel = in.jump_buffered && in.on_ground;

        if (_timer == 0 || blocked || at_edge || jump_cancel) {
            _dashing = false;
            _timer = 0;
            _cooldown_timer = _cooldown;
            step.ended = true;
            return step;
        }

        --_timer;
        step.moving = true;
        return step;
    }

   private:
    int _duration;
    int _cooldown;
    bool _enabled = false;
    bool _dashing = false;
    bool _available = true;
    int _dir = 1;
    int _timer = 0;
    int _cooldown_timer = 0;
};
