#pragma once

#include "bn_fixed.h"

// -----------------------------------------------------------------------------
// door_flee
//
// The Level Devil door that runs away (DoorData::flee_path): it waits at its
// origin, and whenever the duck comes within `range` px of it (on both axes)
// it slides on to the next stop of its path in `frames` frames. At the last
// stop it stays and is an ordinary door. A death puts it back (reset()).
//
// Positions are offsets from the door's origin; the path's points too, like
// TrapData::path. Point is anything with x() / y() (bn::fixed_point in the
// game). No Butano runtime: host-tested in tests/host/test_door_flee.cpp.
// -----------------------------------------------------------------------------
namespace door_flee {

template <typename Point>
class Flee {
   public:
    Flee(const Point* path, int count, int range, int frames)
        : _path(path), _count(count), _range(range), _frames(frames) {}

    // One frame; duck_x / duck_y relative to the door's origin.
    void update(bn::fixed duck_x, bn::fixed duck_y) {
        if (_moving) {
            _advance();
            return;
        }
        if (done()) {
            return;
        }
        const bn::fixed dx = duck_x - _x;
        const bn::fixed dy = duck_y - _y;
        if (dx >= -_range && dx <= _range && dy >= -_range && dy <= _range) {
            _from_x = _x;
            _from_y = _y;
            _frame = 0;
            _moving = true;
            _advance();
        }
    }

    void reset() {
        _stop = 0;
        _frame = 0;
        _moving = false;
        _x = 0;
        _y = 0;
    }

    [[nodiscard]] auto x() const -> bn::fixed { return _x; }
    [[nodiscard]] auto y() const -> bn::fixed { return _y; }
    [[nodiscard]] auto moving() const -> bool { return _moving; }
    // 0 = origin, i = path[i - 1].
    [[nodiscard]] auto stop() const -> int { return _stop; }
    // At its last stop (or no path at all): it never moves again.
    [[nodiscard]] auto done() const -> bool { return _stop >= _count; }

   private:
    void _advance() {
        const Point& to = _path[_stop];
        ++_frame;
        if (_frame >= _frames) {
            _x = to.x();
            _y = to.y();
            ++_stop;
            _moving = false;
            return;
        }
        const bn::fixed ratio = bn::fixed(_frame) / _frames;
        _x = _from_x + (to.x() - _from_x) * ratio;
        _y = _from_y + (to.y() - _from_y) * ratio;
    }

    const Point* _path;
    int _count;
    int _range;
    int _frames;

    int _stop = 0;
    int _frame = 0;
    bool _moving = false;
    bn::fixed _x = 0;
    bn::fixed _y = 0;
    bn::fixed _from_x = 0;
    bn::fixed _from_y = 0;
};

}  // namespace door_flee
