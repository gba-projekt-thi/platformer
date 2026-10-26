#pragma once

// -----------------------------------------------------------------------------
// Cfg::CameraFollow
//
// Tuning of the smoothed camera (CameraRig in camera_rig.h). Lives in its own
// dependency-free header (only built-in types, no Butano) so the host unit
// tests under tests/host/ can check the constants' invariants; cfg.h includes
// it. Every smoothing factor is a fraction NUM/DEN of the remaining gap that
// is closed per frame, so it must stay below 1 (it then never overshoots).
// Horizontally only the look-ahead eases; the follow itself is a dead zone.
// -----------------------------------------------------------------------------

namespace Cfg {

namespace CameraFollow {
// Horizontal dead zone: the duck may move DEADZONE_X px either way from the
// camera's focus before the camera moves; beyond it the camera moves exactly
// as far as the duck (no easing), so the duck keeps its spot on the screen
// and nothing slides on after it stops - small steps do not scroll at all.
inline constexpr int DEADZONE_X = 8;
// Vertical follow is lazier, so a jump does not shake the picture.
inline constexpr int FOLLOW_Y_NUM = 1;
inline constexpr int FOLLOW_Y_DEN = 8;

// Look-ahead: the camera aims LOOKAHEAD_PX in front of the duck, in its
// direction of travel (kept while it stands still), and eases there slowly.
inline constexpr int LOOKAHEAD_PX = 20;
inline constexpr int LOOKAHEAD_NUM = 1;
inline constexpr int LOOKAHEAD_DEN = 24;
// Horizontal speed (px/frame) from which a movement counts as a direction,
// and for how many frames in a row: a tap never moves the look-ahead.
inline constexpr int LOOKAHEAD_MIN_SPEED = 1;
inline constexpr int LOOKAHEAD_HOLD_FRAMES = 15;

// Look up / down: holding the D-Pad up or down for LOOK_Y_HOLD_FRAMES moves
// the camera LOOK_Y_PX that way (only visible in levels taller than the
// screen: LevelData::world_height > 160).
inline constexpr int LOOK_Y_PX = 32;
inline constexpr int LOOK_Y_HOLD_FRAMES = 24;
inline constexpr int LOOK_Y_NUM = 1;
inline constexpr int LOOK_Y_DEN = 10;
}  // namespace CameraFollow

}  // namespace Cfg
