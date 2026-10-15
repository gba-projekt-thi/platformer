#pragma once

// -----------------------------------------------------------------------------
// DashTrail
//
// Pure bookkeeping of the dash's afterimage ghosts: when a ghost is spawned
// while dashing, which slot it takes, how long it lives and when it is drawn
// (flickering, to fake transparency without touching the blending setup the
// scene fades use). Split out of Player so it has no Butano dependency and can
// be unit-tested on the host (see tests/host/test_dash_trail.cpp); Player owns
// one sprite per slot and mirrors this state onto them.
//
// Per-frame protocol (Player::update()):
//   - dash started this frame:      begin()
//   - every frame while dashing:    slot = on_dash_frame(); spawn if slot >= 0
//   - every frame:                  tick(), then show/hide/free the sprites
//                                   from active(slot) / visible(slot)
// -----------------------------------------------------------------------------

class DashTrail {
   public:
    static constexpr int SLOTS = 4;
    // Frames a ghost lives after it was spawned (SLOTS must cover
    // ceil(LIFE_FRAMES / SPAWN_EVERY) simultaneous ghosts).
    static constexpr int LIFE_FRAMES = 6;
    // A ghost is spawned on every SPAWN_EVERY-th dash frame, starting with
    // the first one.
    static constexpr int SPAWN_EVERY = 2;

    // A new dash starts: the first dash frame spawns a ghost at once.
    void begin() { _since_spawn = SPAWN_EVERY; }

    // Call once per dash frame. Returns the slot to spawn a ghost in, or -1
    // when this frame does not spawn one. Takes a free slot, else the one with
    // the least life left.
    auto on_dash_frame() -> int {
        if (_since_spawn < SPAWN_EVERY) {
            ++_since_spawn;
            return -1;
        }
        _since_spawn = 1;

        int slot = 0;
        for (int i = 1; i < SLOTS; ++i) {
            if (_life[i] < _life[slot]) {
                slot = i;
            }
        }
        _life[slot] = LIFE_FRAMES;
        return slot;
    }

    // Ages every ghost by one frame.
    void tick() {
        for (int& life : _life) {
            if (life > 0) {
                --life;
            }
        }
    }

    [[nodiscard]] auto active(int slot) const -> bool {
        return _life[slot] > 0;
    }

    // Drawn on odd remaining life only: a ghost flickers on and off on its
    // way out, and is hidden on the frame it is spawned (it sits exactly
    // behind the duck then).
    [[nodiscard]] auto visible(int slot) const -> bool {
        return _life[slot] > 0 && (_life[slot] % 2) == 1;
    }

    [[nodiscard]] auto life(int slot) const -> int { return _life[slot]; }

    void clear() {
        for (int& life : _life) {
            life = 0;
        }
        _since_spawn = 0;
    }

   private:
    int _life[SLOTS] = {};
    int _since_spawn = 0;
};
