#pragma once

#include "bn_fixed.h"

#include "door_flee.h"

// -----------------------------------------------------------------------------
// FleeingPlatforms
//
// Platforms that run away (PlatformData::flee_path): whenever the duck comes
// within flee_range px of one it slides on to the next stop of its path, like
// the runaway door (door_flee.h) - the duck lands where the platform was. A
// death puts them back (reset()). LevelManager owns the bodies and pictures;
// this moves them. Templated over the body / picture / path point types so it
// is host-tested with fakes (tests/host/test_fleeing_platforms.cpp).
// -----------------------------------------------------------------------------
template <typename Body, typename Picture, typename Point, int Capacity>
class FleeingPlatforms {
   public:
    void add(
        Body& body,
        Picture& picture,
        bn::fixed x,
        bn::fixed y,
        const Point* path,
        int count,
        int range,
        int frames) {
        if (_count < Capacity) {
            _entries[_count] = Entry{
                &body, &picture, x, y,
                door_flee::Flee<Point>(path, count, range, frames)};
            ++_count;
        }
    }

    // One frame, with the duck's position.
    void update(bn::fixed duck_x, bn::fixed duck_y) {
        for (int i = 0; i < _count; ++i) {
            Entry& e = _entries[i];
            if (e.flee.done() && !e.flee.moving()) {
                continue;
            }
            e.flee.update(duck_x - e.x, duck_y - e.y);
            _place(e);
        }
    }

    // Death or restart: every platform back to where it started.
    void reset() {
        for (int i = 0; i < _count; ++i) {
            _entries[i].flee.reset();
            _place(_entries[i]);
        }
    }

    void clear() { _count = 0; }
    [[nodiscard]] auto empty() const -> bool { return _count == 0; }

   private:
    struct Entry {
        Body* body;
        Picture* picture;
        bn::fixed x;
        bn::fixed y;
        door_flee::Flee<Point> flee;
    };

    static void _place(Entry& e) {
        e.body->pos.move(e.x + e.flee.x(), e.y + e.flee.y());
        e.picture->pos.move(e.x + e.flee.x(), e.y + e.flee.y());
    }

    Entry _entries[Capacity] = {};
    int _count = 0;
};
