#pragma once
#include <cstdint>
#include "bn_fixed.h"

namespace Cfg {

namespace Layer {
inline constexpr uint16_t UN_HITTABLE = 0x0000;
inline constexpr uint16_t PLAYER = 0x0001;
inline constexpr uint16_t TRAP = 0x0002;
inline constexpr uint16_t PLATFORM = 0x0004;
inline constexpr uint16_t TRIGGER = 0x0008;
inline constexpr uint16_t DOOR = 0x0010;
}  // namespace Layer

namespace Screen {
inline constexpr int WIDTH = 240;
inline constexpr int HEIGHT = 160;
inline constexpr int HORIZONTAL_EDGE = 110;  // default bounce edge for a
                                             // 240-wide (non-scrolling)
                                             // level = 240/2 - EDGE_MARGIN
inline constexpr int EDGE_MARGIN = 10;       // margin kept from the true
                                             // world edge, per level
}  // namespace Screen

namespace Player {
// Physics
//
// ACCELERATION was bumped from 0.3 to 0.38 for a snappier feel: the duck
// reaches MAX_SPEED noticeably sooner (~100ms vs ~117ms), which reads as
// more responsive without changing top speed or the jump arc's climb
// height. It also very slightly *increases* max horizontal jump distance
// (~52px -> ~54px), so every existing level stays within the same
// provably-clearable envelope described in levels_common.h - if anything
// with a hair more margin than before. Because MAX_SPEED (not
// acceleration) dominates how quickly the duck covers any real distance,
// this change is imperceptible for how much runway the player has to
// react to a just-triggered hazard - see the comment on DECELERATION
// below and LevelData::ground_friction in level_structure.h for the
// knob that's actually meant to change stopping feel per level.
inline constexpr bn::fixed ACCELERATION = 0.38;
// Braking (releasing input) previously reused ACCELERATION for both
// speeding up and slowing down. Split out as its own constant, kept at
// the pre-tuning value, so the ACCELERATION bump above doesn't also make
// stopping snappier - the duck still needs the same runway to come to a
// full stop after a trigger fires as it always has. Levels can loosen
// this per-stage via LevelData::ground_friction (a multiplier on this
// value, default 1) for a deliberately slippery floor - see
// PlayerLocomotion::set_ground_friction().
inline constexpr bn::fixed DECELERATION = 0.3;
inline constexpr int MAX_SPEED = 2;
inline constexpr int JUMP_SPEED = -3;
inline constexpr bn::fixed GRAVITY = 0.22;
inline constexpr int MAX_FALL_SPEED = 3;

// Animations
inline constexpr int RIGHT_FRAMES[2] = {0, 1};
inline constexpr int LEFT_FRAMES[2] = {4, 5};
inline constexpr int JUMP_RIGHT_FRAMES[2] = {2, 3};
inline constexpr int JUMP_LEFT_FRAMES[2] = {6, 7};
inline constexpr int IDLE_FRAME = 8;
inline constexpr int BACK_FRAME = 9;
inline constexpr int PLAYER_TILE_CACHE_SIZE = 10;
inline constexpr int ANIMATION_FRAME_COUNT = 4;
inline constexpr int WAIT_UPDATE = 4;

// Misc.
inline constexpr int DEATH_HEIGHT = 100;
inline constexpr int COYOTE_FRAMES = 4;
inline constexpr int JUMP_BUFFER_FRAMES = 6;
}  // namespace Player

namespace DeathCounter {
inline constexpr int X = -110;
inline constexpr int Y = -70;
inline constexpr unsigned int STRING_LEN = 16;
inline constexpr unsigned int COUNT_LEN = 8;
}  // namespace DeathCounter

namespace Timer {
inline constexpr int X = 50;
inline constexpr int Y = -70;
}  // namespace Timer

namespace NewBestBanner {
// X chosen to fit the longest line ("Press A to continue", ~152px)
// roughly centered; the shorter lines above it sit left of center.
inline constexpr int X = -76;
inline constexpr int Y = -50;
}  // namespace NewBestBanner

namespace Summary {
// Same X as NewBestBanner - both center a similar-length longest line
// ("Press A to continue"). Five lines total (title, time, deaths,
// no-death clears, prompt); the prompt gets an extra LINE_SPACING gap
// above it to read as a separate call-to-action, not a stat.
inline constexpr int X = -76;
inline constexpr int Y = -50;
inline constexpr int LINE_SPACING = 16;
}  // namespace Summary

namespace HardMode {
// Applied to MOVING (velocity/max_vel), CHASE (chase_speed) and AMBUSH
// (speed) trap values in TrapFactory::create() when GameState::
// hard_mode_enabled is set. PATH traps are deliberately left alone -
// their speed lives in path_waits (an unsigned frame count), which
// needs its own division/rounding-edge-case handling; a separate
// change if wanted later.
inline constexpr bn::fixed SPEED_MULTIPLIER = 1.35;
}  // namespace HardMode

namespace Level {
namespace Limits {
inline constexpr unsigned int PLATFORMS = 32;
inline constexpr unsigned int PLATFORM_BODIES = 32;
inline constexpr unsigned int TRIGGERS = 16;
inline constexpr unsigned int TOTAL_TRAPS = 32;
}  // namespace Limits
}  // namespace Level

inline constexpr unsigned int MAX_ANIMATION_FRAMES = 8;

namespace ZOrder {
inline constexpr int PAUSE_MENU = -100;
inline constexpr int DEATH_COUNTER = -100;
inline constexpr int TIMER = -100;
inline constexpr int DOOR = 1;
}  // namespace ZOrder

namespace PauseMenu {
inline constexpr int X = -30;
inline constexpr int Y_TITLE = -28;
inline constexpr int Y_OPTION_0 = -8;
inline constexpr int OPTION_SPACING = 16;
}  // namespace PauseMenu

namespace StartScreen {
inline constexpr int X = -80;
inline constexpr int Y = -10;
inline constexpr int SAVE_SLOT_COUNT = 3;
}  // namespace StartScreen

namespace Physics {
inline constexpr bn::fixed NO_MAX_VELOCITY = 0;
}

namespace Sleep {
inline constexpr int DEATH = 20;           // 0.33s
inline constexpr int FINISHED_GAME = 600;  // 10s
inline constexpr int KISSING_SCENE = 300;  // 5s
}  // namespace Sleep

}  // namespace Cfg