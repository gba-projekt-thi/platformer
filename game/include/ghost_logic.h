#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// -----------------------------------------------------------------------------
// ghost
//
// The ghost of the best run of a level: a see-through duck that replays what
// the fastest run of this power-on session looked like. Kept in RAM only (no
// save data); everything here is Butano-free and host-tested
// (tests/host/test_ghost_logic.cpp). ghost_session.cpp is the thin glue that
// feeds it the duck and draws the ghost.
//
// What is recorded: the duck's position, facing and picture, every
// SAMPLE_FRAMES-th frame of the level clock (LevelManager::level_frame_count),
// deaths and respawns included - a recording of what was seen. The ghost
// interpolates the frames in between.
//
// Encoding, per sample, delta to the previous one:
//   dx, dy (int8), pose           3 bytes   pose = picture | flip << 7
//   0x80, pose, x, y (int16 LE)   6 bytes   a jump (respawn, checkpoint) or the
//                                           first sample: absolute position
// Nearly every sample takes 3 bytes: 1.5 bytes per frame, 90 per second.
//
// RAM: Store holds STORE_BYTES for all levels together (one ghost per level
// key, packed back to back, so short levels leave room for long ones) and
// Recorder RECORD_BYTES for the run being played. 48 KiB hold 32768 frames,
// about 9 minutes of ghosts - every one of the 26 levels at 21 s - for 19% of
// EWRAM; the fastest runs of all 26 levels (game/replays) add up to 10000
// frames, 15 KB.
// -----------------------------------------------------------------------------

namespace ghost {

inline constexpr int SAMPLE_FRAMES = 2;
inline constexpr size_t STORE_BYTES = 48 * 1024;
inline constexpr size_t RECORD_BYTES = 6 * 1024;  // 4000+ frames, over a minute
// Levels a key can name: GameState::MAX_LEVELS, twice for the mirrored copies
// of the levels (key_for()).
inline constexpr unsigned LEVELS = 32;
inline constexpr unsigned KEYS = 2 * LEVELS;

// A step of more than this between two samples is a jump (no interpolation).
inline constexpr int MAX_STEP = 48;
inline constexpr uint8_t JUMP = 0x80;
inline constexpr uint8_t FLIP = 0x80;

using Key = uint8_t;

// The ghost of a level; the mirrored version of a level gets its own.
constexpr auto key_for(unsigned level_index, bool mirrored = false) -> Key {
    return Key(level_index + (mirrored ? LEVELS : 0));
}

struct Pose {
    int16_t x = 0;
    int16_t y = 0;
    uint8_t tile = 0;  // < 128
    bool flip = false;
};

namespace detail {

// Writes the sample after `prev` (nullptr = the first) to `out`; returns its
// length.
inline auto encode(const Pose* prev, const Pose& pose, uint8_t* out) -> size_t {
    const uint8_t look = uint8_t(pose.tile | (pose.flip ? FLIP : 0));
    if (prev != nullptr) {
        const int dx = pose.x - prev->x;
        const int dy = pose.y - prev->y;
        if (dx >= -MAX_STEP && dx <= MAX_STEP && dy >= -MAX_STEP &&
            dy <= MAX_STEP) {
            out[0] = uint8_t(int8_t(dx));
            out[1] = uint8_t(int8_t(dy));
            out[2] = look;
            return 3;
        }
    }
    out[0] = JUMP;
    out[1] = look;
    out[2] = uint8_t(pose.x & 0xFF);
    out[3] = uint8_t((uint16_t(pose.x) >> 8) & 0xFF);
    out[4] = uint8_t(pose.y & 0xFF);
    out[5] = uint8_t((uint16_t(pose.y) >> 8) & 0xFF);
    return 6;
}

// Reads the sample at `data[pos]` after `prev`; returns its length (0 = it
// does not fit in `size`). `jump` tells an absolute sample.
inline auto decode(
    const uint8_t* data,
    size_t size,
    size_t pos,
    const Pose& prev,
    Pose& out,
    bool& jump) -> size_t {
    if (pos >= size) {
        return 0;
    }
    jump = data[pos] == JUMP;
    if (jump) {
        if (pos + 6 > size) {
            return 0;
        }
        out.tile = data[pos + 1] & uint8_t(~FLIP);
        out.flip = (data[pos + 1] & FLIP) != 0;
        out.x = int16_t(uint16_t(data[pos + 2]) | uint16_t(data[pos + 3]) << 8);
        out.y = int16_t(uint16_t(data[pos + 4]) | uint16_t(data[pos + 5]) << 8);
        return 6;
    }
    if (pos + 3 > size) {
        return 0;
    }
    out.x = int16_t(prev.x + int8_t(data[pos]));
    out.y = int16_t(prev.y + int8_t(data[pos + 1]));
    out.tile = data[pos + 2] & uint8_t(~FLIP);
    out.flip = (data[pos + 2] & FLIP) != 0;
    return 3;
}

}  // namespace detail

// -----------------------------------------------------------------------------
// Recorder: the run being played
// -----------------------------------------------------------------------------

template <size_t Capacity>
class Recorder {
   public:
    void clear() {
        _size = 0;
        _samples = 0;
        _failed = false;
    }

    // The duck's pose after `frame` frames of the level clock. Call it for
    // every frame the clock advances to; only every SAMPLE_FRAMES-th is kept.
    void sample(uint32_t frame, const Pose& pose) {
        if (frame % SAMPLE_FRAMES != 0 || _failed) {
            return;
        }
        // A frame skipped or seen twice, or no room left: the recording would
        // run out of step.
        if (frame / SAMPLE_FRAMES != _samples || _size + 6 > Capacity) {
            _failed = true;
            return;
        }
        _size += detail::encode(
            _samples != 0 ? &_last : nullptr, pose, _data + _size);
        _last = pose;
        ++_samples;
    }

    // False: the run was too long for the buffer (or had a gap).
    [[nodiscard]] auto usable() const -> bool {
        return !_failed && _samples != 0;
    }
    [[nodiscard]] auto data() const -> const uint8_t* { return _data; }
    [[nodiscard]] auto size() const -> size_t { return _size; }

   private:
    uint8_t _data[Capacity];
    size_t _size = 0;
    uint32_t _samples = 0;
    Pose _last;
    bool _failed = false;
};

// -----------------------------------------------------------------------------
// Playback: a stored ghost, asked for the pose of one frame after the other
// -----------------------------------------------------------------------------

class Playback {
   public:
    Playback() = default;
    // `frames`: the clock at which the run finished; the ghost is gone then.
    Playback(const uint8_t* data, size_t size, uint32_t frames)
        : _data(data), _size(size), _frames(frames) {
        rewind();
    }

    [[nodiscard]] auto valid() const -> bool { return _data != nullptr; }

    // The pose to draw when the level clock stands at `frame`; false once the
    // ghost's run is over. Frames normally come one after the other; going
    // back starts over.
    auto pose_at(uint32_t frame, Pose& out) -> bool {
        if (_data == nullptr || frame >= _frames) {
            return false;
        }
        const uint32_t index = frame / SAMPLE_FRAMES;
        if (index < _index) {
            rewind();
        }
        while (_index < index) {
            Pose next;
            bool jump = false;
            const size_t length =
                detail::decode(_data, _size, _pos, _cur, next, jump);
            if (length == 0) {
                break;  // the recording ends here: hold the last pose
            }
            _cur = next;
            _pos += length;
            ++_index;
        }
        out = _cur;
        if (frame % SAMPLE_FRAMES != 0 && _index == index) {
            // Halfway to the next sample, unless that one is a jump.
            Pose next;
            bool jump = false;
            if (detail::decode(_data, _size, _pos, _cur, next, jump) != 0 &&
                !jump) {
                out.x = int16_t((_cur.x + next.x) / 2);
                out.y = int16_t((_cur.y + next.y) / 2);
            }
        }
        return true;
    }

   private:
    void rewind() {
        _index = 0;
        _cur = Pose{};
        bool jump = false;
        _pos = detail::decode(_data, _size, 0, Pose{}, _cur, jump);
    }

    const uint8_t* _data = nullptr;
    size_t _size = 0;
    uint32_t _frames = 0;
    size_t _pos = 0;      // after the sample `_cur`
    uint32_t _index = 0;  // of `_cur`
    Pose _cur;
};

// -----------------------------------------------------------------------------
// Store: the best run of every level, packed in one buffer
// -----------------------------------------------------------------------------

template <size_t Bytes, unsigned Keys>
class Store {
    static_assert(Bytes <= 0xFFFF);

   public:
    [[nodiscard]] auto has(Key key) const -> bool {
        return key < Keys && _entries[key].size != 0;
    }
    [[nodiscard]] auto frames(Key key) const -> uint32_t {
        return has(key) ? _entries[key].frames : 0;
    }
    [[nodiscard]] auto bytes_used() const -> size_t { return _used; }

    // A run of `frames` frames beats the level's ghost: there is none yet, or
    // it is slower.
    [[nodiscard]] auto beats(Key key, uint32_t frames) const -> bool {
        return key < Keys && frames > 0 &&
               (!has(key) || frames < _entries[key].frames);
    }

    // Stores the run if it beats the level's ghost and fits (the old ghost
    // stays when it does not). True: it is the level's ghost now.
    auto offer(Key key, uint32_t frames, const uint8_t* data, size_t size)
        -> bool {
        if (!beats(key, frames) || size == 0) {
            return false;
        }
        const size_t old_size = has(key) ? _entries[key].size : 0;
        if (size > Bytes - _used + old_size) {
            return false;
        }
        if (old_size != 0) {
            remove(key);
        }
        std::memcpy(_data + _used, data, size);
        _entries[key] = {frames, uint16_t(_used), uint16_t(size)};
        _used += size;
        return true;
    }

    [[nodiscard]] auto open(Key key) const -> Playback {
        if (!has(key)) {
            return Playback();
        }
        const Entry& e = _entries[key];
        return Playback(_data + e.offset, e.size, e.frames);
    }

   private:
    struct Entry {
        uint32_t frames;
        uint16_t offset;
        uint16_t size;
    };

    // Closes the gap the key's ghost leaves.
    void remove(Key key) {
        const Entry gone = _entries[key];
        std::memmove(
            _data + gone.offset, _data + gone.offset + gone.size,
            _used - gone.offset - gone.size);
        for (unsigned i = 0; i < Keys; ++i) {
            if (_entries[i].size != 0 && _entries[i].offset > gone.offset) {
                _entries[i].offset = uint16_t(_entries[i].offset - gone.size);
            }
        }
        _used -= gone.size;
        _entries[key] = {0, 0, 0};
    }

    uint8_t _data[Bytes];
    Entry _entries[Keys] = {};
    size_t _used = 0;
};

}  // namespace ghost
