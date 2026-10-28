#pragma once

// -----------------------------------------------------------------------------
// StrikeTargets
//
// The bodies a pogo strike can hurt - bosses, the one enemy the duck can
// beat. Traps only bounce the duck; a body registered here is also told it
// was struck. The registry maps the collision body (what the strike's probe
// finds) to its Strikeable owner, so the duck needs no knowledge of bosses.
// No Butano: host-tested in tests/host/test_strike_targets.cpp.
// -----------------------------------------------------------------------------
class Strikeable {
   public:
    virtual void on_struck() = 0;

   protected:
    ~Strikeable() = default;
};

class StrikeTargets {
   public:
    static constexpr int CAPACITY = 4;

    static auto instance() -> StrikeTargets& {
        static StrikeTargets targets;
        return targets;
    }

    void add(const void* body, Strikeable& target) {
        if (_count < CAPACITY) {
            _bodies[_count] = body;
            _targets[_count] = &target;
            ++_count;
        }
    }

    void remove(const void* body) {
        for (int i = 0; i < _count; ++i) {
            if (_bodies[i] == body) {
                --_count;
                _bodies[i] = _bodies[_count];
                _targets[i] = _targets[_count];
                return;
            }
        }
    }

    // Tells the owner of `body` it was struck; false if it is no target.
    auto strike(const void* body) -> bool {
        for (int i = 0; i < _count; ++i) {
            if (_bodies[i] == body) {
                _targets[i]->on_struck();
                return true;
            }
        }
        return false;
    }

   private:
    const void* _bodies[CAPACITY] = {};
    Strikeable* _targets[CAPACITY] = {};
    int _count = 0;
};
