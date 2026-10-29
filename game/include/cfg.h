#pragma once
#include <cstdint>
#include "bn_fixed.h"
#include "cfg_boss.h"
#include "cfg_camera.h"
#include "cfg_dash.h"
#include "cfg_death_fx.h"
#include "cfg_pogo.h"
#include "cfg_wall_jump.h"

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
// ACCELERATION 0.38: the duck reaches MAX_SPEED in ~100 ms, which reads as
// responsive without changing top speed or the jump arc's climb height. The
// max horizontal jump distance (~54px) stays inside the envelope described in
// levels_common.h. Stopping feel is tuned per level with
// LevelData::ground_friction (see DECELERATION below).
inline constexpr bn::fixed ACCELERATION = 0.38;
// Braking (releasing input) is separate from ACCELERATION so a snappy start
// does not also make stopping snappier: the duck needs the same runway to
// stop after a trigger fires. Levels can loosen it via
// LevelData::ground_friction (a multiplier, default 1) for a slippery floor -
// see PlayerLocomotion::set_ground_friction().
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
// Wall slide: back to the wall (drawn for a wall on the right; flipped like
// every frame for one on the left), dust drifting up along it.
inline constexpr int SLIDE_FRAMES[2] = {10, 11};
inline constexpr int SLIDE_WAIT = 6;  // frames per dust picture
inline constexpr int PLAYER_TILE_CACHE_SIZE = 12;
inline constexpr int ANIMATION_FRAME_COUNT = 4;
inline constexpr int WAIT_UPDATE = 4;

// Collision body (centered on the duck's position). The 16x16 picture is
// larger, so it is drawn SPRITE_OFFSET_Y higher: its feet (the lowest opaque
// row of ente.bmp) then rest on the body's bottom, i.e. on the platform.
// Visual only; tools/Check_hitboxes.py checks the feet.
inline constexpr int BODY_WIDTH = 8;
inline constexpr int BODY_HEIGHT = 8;
inline constexpr int SPRITE_OFFSET_Y = -4;

// Misc.
inline constexpr int DEATH_HEIGHT = 100;
inline constexpr int COYOTE_FRAMES = 4;
inline constexpr int JUMP_BUFFER_FRAMES = 6;
}  // namespace Player

namespace Hint {
// One-time level hint (LevelData::intro_hint), see hint_logic.h.
inline constexpr int FRAMES = 300;  // visible for 5s unless dismissed
inline constexpr int Y = -48;       // below the HUD row (y = -70), above play
}  // namespace Hint

namespace ClearMoment {
// "Clear!" of the level-clear moment (clear_logic.h), above the door's row.
inline constexpr int Y = -24;
}  // namespace ClearMoment

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
// hard_mode_enabled is set. PATH traps get the same factor too, but their
// speed lives in path_waits (an unsigned frame count per node), so it is
// divided instead: trap_timing::scaled_frames(path_waits, percent).
inline constexpr bn::fixed SPEED_MULTIPLIER = 1.35;
}  // namespace HardMode

namespace AssistMode {
// Mirror image of HardMode: applied to the same MOVING/CHASE/AMBUSH values
// in TrapFactory::create(), and to PATH traps via trap_timing::scaled_frames()
// (half speed = twice the frames per node).
inline constexpr bn::fixed TRAP_SPEED_MULTIPLIER = 0.5;
// Forgiveness windows, replacing Cfg::Player::COYOTE_FRAMES /
// JUMP_BUFFER_FRAMES while active. Jump speed, gravity and acceleration
// are untouched, so the jump envelope level layouts rely on is unchanged.
inline constexpr int COYOTE_FRAMES = 8;
inline constexpr int JUMP_BUFFER_FRAMES = 12;
}  // namespace AssistMode

namespace Level {
namespace Limits {
inline constexpr unsigned int PLATFORMS = 32;
inline constexpr unsigned int PLATFORM_BODIES = 32;
inline constexpr unsigned int TRIGGERS = 16;
inline constexpr unsigned int TOTAL_TRAPS = 32;
inline constexpr unsigned int CHECKPOINTS = 1;
// Cap on the sprites of one level's platforms, traps, checkpoints and
// triggers with a switch sprite, together: the per-kind limits above alone
// allow more than the 72 SpriteRegistry slots and the 128 hardware sprites
// hold (2 duck/door + this + 4 dash trail + 1 ghost + 20 HUD + 60 hint and
// banner, or the 59 of the pause overlay, = 125 of the 128). Checked in
// LevelManager::_validate_level() and by tools/Check_sprite_budget.py.
inline constexpr unsigned int ENTITY_SPRITES = 38;
}  // namespace Limits
}  // namespace Level

inline constexpr unsigned int MAX_ANIMATION_FRAMES = 8;

namespace ZOrder {
inline constexpr int PAUSE_MENU = -100;
inline constexpr int DEATH_COUNTER = -100;
inline constexpr int TIMER = -100;
inline constexpr int DOOR = 1;
inline constexpr int CHECKPOINT = 1;  // behind the duck, like the door
}  // namespace ZOrder

namespace PauseMenu {
inline constexpr int X = -30;
inline constexpr int Y_TITLE = -28;
inline constexpr int Y_OPTION_0 = -8;
inline constexpr int OPTION_SPACING = 16;
// Black fade over the level (blending-enabled sprites and backgrounds) while
// the menu is open, so the white text stays readable on bright backgrounds.
inline constexpr bn::fixed DIM_ALPHA = 0.6;
}  // namespace PauseMenu

namespace StartScreen {
inline constexpr int X = -80;
inline constexpr int Y = -10;
inline constexpr int SAVE_SLOT_COUNT = 3;
// Save slots plus the "Controls" entry below them.
inline constexpr int ENTRY_COUNT = SAVE_SLOT_COUNT + 1;
}  // namespace StartScreen

namespace Statistics {
// StatisticsScene: a title, statistics::VISIBLE_LINES lines of LINE_SPACING
// from FIRST_LINE_Y, and the hint below them.
inline constexpr int X = -100;
inline constexpr int TITLE_Y = -70;
inline constexpr int FIRST_LINE_Y = -50;
inline constexpr int LINE_SPACING = 16;
inline constexpr int HINT_Y = 68;
}  // namespace Statistics

namespace Story {
// StoryScene: up to story::MAX_LINES centered lines from FIRST_LINE_Y, the
// prompt below them (checked to fit by tests/host/test_story.cpp), over the
// dimmed world background. The duck (2x scaled) stands below the prompt.
inline constexpr int FIRST_LINE_Y = -42;
inline constexpr int LINE_SPACING = 18;
inline constexpr int PROMPT_Y = 30;
inline constexpr int DUCK_X = 0;
inline constexpr int DUCK_Y = 58;
// Black fade of the picture: the village and the bathtub are busy behind text.
inline constexpr bn::fixed DIM_ALPHA = 0.75;
}  // namespace Story

namespace LevelSelect {
// One row per level of the chosen world, from StartScreen::Y down; a world
// with more levels starts higher (menu_nav::first_row_y()) so its last row
// still sits at LAST_ROW_Y at most.
inline constexpr int ROW_SPACING = 16;
inline constexpr int LAST_ROW_Y = 70;
// The Mirror Mode line (Mirror Mode: see modes::mirror_active()) and below it
// the selected level's goals ("[*] No death  [+] <1:30"), both above the first
// row of even the longest world (8 rows, the first at y = -42).
inline constexpr int MIRROR_Y = -72;
inline constexpr int GOALS_X = -90;
inline constexpr int GOALS_Y = -57;
}  // namespace LevelSelect

namespace Physics {
inline constexpr bn::fixed NO_MAX_VELOCITY = 0;
}

namespace Sleep {
inline constexpr int DEATH = 20;           // 0.33s
inline constexpr int FINISHED_GAME = 600;  // 10s
inline constexpr int KISSING_SCENE = 300;  // 5s
}  // namespace Sleep

}  // namespace Cfg