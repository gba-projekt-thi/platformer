#pragma once

// -----------------------------------------------------------------------------
// ghost_session
//
// The ghost of the best run of every level, for this power-on session (RAM
// only, not saved; the rules are in ghost_logic.h). LevelScene calls it around
// the level; the pause menu's Options switch it on and off. The ghost is a pale
// copy of the duck, drawn by a Sprite of the SpriteRegistry (so it scrolls
// with the world); it has no body and no effect on the game. Nothing here runs
// in the replay ROM (it plays through LevelManager only) or the record build.
// -----------------------------------------------------------------------------

class Player;

namespace ghost_session {

// The Options row "Ghost": whether the ghost is shown (recording goes on
// either way). On at power-on.
[[nodiscard]] bool enabled();
void set_enabled(bool enabled);

// The level is loaded, the clock stands at 0: start recording; the level's
// ghost, if there is one, will play.
void level_started(Player& player, unsigned level_index, bool mirrored);
// Before a frame of the level: draws the ghost where it was when the clock
// reached `level_frame` + 1.
void before_frame(unsigned level_frame);
// After it: records the duck at `level_frame`.
void after_frame(Player& player, unsigned level_frame);
// The door was reached after `level_frame` frames: the run becomes the
// level's ghost if it is faster than the one there. `records_allowed`:
// LevelManager::records_allowed() (no Assist Mode runs).
void level_finished(unsigned level_frame, bool records_allowed);
// The scene ends: the ghost's sprite goes.
void level_left();

}  // namespace ghost_session
