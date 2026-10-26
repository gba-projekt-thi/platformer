#pragma once

// -----------------------------------------------------------------------------
// ambush
//
// The motion of a lurking trap (TrapType::AMBUSH): dormant until the duck
// comes within `range` (horizontally, measured by the caller), then it
// lunges `speed` per frame along its axis for `duration` frames, comes back
// at the same speed and waits again. AmbushTrap applies the offset along x
// (a disguised enemy lunging sideways) or y (ambush_vertical: arms or spikes
// shooting out of the ground). T is int or bn::fixed; no Butano runtime -
// host-tested in tests/host/test_ambush_logic.cpp.
// -----------------------------------------------------------------------------
namespace ambush {

enum class State { Dormant, Lunging, Returning };

template <typename T>
class Motion {
   public:
    Motion(T range, T speed, unsigned duration)
        : _range(range), _speed(speed), _duration(duration) {}

    // One frame; duck_distance = how far the duck is from the trap.
    void update(T duck_distance) {
        if (_state == State::Dormant) {
            if (duck_distance <= _range) {
                _state = State::Lunging;  // moves from the next frame on
                _elapsed = 0;
            }
            return;
        }
        if (_state == State::Lunging) {
            _offset += _speed;
            if (++_elapsed >= _duration) {
                _state = State::Returning;
            }
            return;
        }
        const T step = _speed < 0 ? -_speed : _speed;
        if (_offset > 0) {
            _offset = _offset > step ? _offset - step : T(0);
        } else {
            _offset = -_offset > step ? _offset + step : T(0);
        }
        if (_offset == 0) {
            _state = State::Dormant;
        }
    }

    void reset() {
        _state = State::Dormant;
        _offset = 0;
        _elapsed = 0;
    }

    [[nodiscard]] auto offset() const -> T { return _offset; }
    [[nodiscard]] auto state() const -> State { return _state; }

   private:
    T _range;
    T _speed;
    unsigned _duration;
    State _state = State::Dormant;
    T _offset = 0;
    unsigned _elapsed = 0;
};

}  // namespace ambush
