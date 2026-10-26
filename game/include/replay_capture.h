#pragma once

// -----------------------------------------------------------------------------
// replay_capture
//
// `make record` builds the game with PLATFORMER_RECORD: every level the duck
// finishes stores the buttons of that attempt in SRAM (replay.h), from where
// tools/replays.py imports them. In every other build these calls do nothing.
// -----------------------------------------------------------------------------

class Player;

namespace replay_capture {

#if defined(PLATFORMER_RECORD)

// A level starts: record the duck's buttons from now on.
void level_started(Player& player, unsigned level_index);
// Restart Level: the recording of this level starts over.
void level_restarted();
// The door was reached: store the recording.
void level_finished();

#else

inline void level_started(Player&, unsigned) {}
inline void level_restarted() {}
inline void level_finished() {}

#endif

}  // namespace replay_capture
