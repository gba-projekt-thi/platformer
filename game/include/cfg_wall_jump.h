#pragma once

// -----------------------------------------------------------------------------
// Cfg::WallJump
//
// Wall slide and wall jump tuning (Hollow Knight's Mantis Claw). Dependency
// free like cfg_dash.h so the host tests can check it; cfg.h includes it.
// -----------------------------------------------------------------------------

namespace Cfg {

namespace WallJump {
// Falling while holding toward a wall the duck touches is capped at
// SLIDE_SPEED px/frame (instead of Player::MAX_FALL_SPEED).
inline constexpr int SLIDE_SPEED = 1;
// A jump in the air at a wall: Player::JUMP_SPEED up and PUSH_SPEED px/frame
// away from the wall. For LOCK_FRAMES the D-pad is ignored, so the duck
// really leaves the wall even while the player still holds toward it
// (landing ends the lock early).
inline constexpr int PUSH_SPEED = 2;
inline constexpr int LOCK_FRAMES = 8;
// Index into WorldIndex::WORLDS of the first world with wall jumps
// (4 = World 5). Earlier levels were authored without them.
inline constexpr int UNLOCK_WORLD = 4;
}  // namespace WallJump

}  // namespace Cfg
