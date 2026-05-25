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
inline constexpr int HORIZONTAL_EDGE = 110;
}

namespace Player {
// Physics
inline constexpr bn::fixed ACCELERATION = 0.3;
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

namespace Level {
namespace Limits {
inline constexpr unsigned int PLATFORMS = 32;
inline constexpr unsigned int PLATFORM_BODIES = 32;
inline constexpr unsigned int TRIGGERS = 16;
inline constexpr unsigned int BASE_TRAPS = 16;
inline constexpr unsigned int MOVING_TRAPS = 16;
inline constexpr unsigned int PATH_TRAPS = 8;
inline constexpr unsigned int RESETTABLES = 16;
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
inline constexpr int Y_0 = -20;
inline constexpr int Y_1 = -4;
inline constexpr int Y_2 = 12;
}  // namespace PauseMenu

namespace Physics {
inline constexpr bn::fixed NO_MAX_VELOCITY = 0;
}

namespace Sleep {
inline constexpr int DEATH = 20;           // 0.33s
inline constexpr int DOOR_REACHED = 60;    // 1s
inline constexpr int FINISHED_GAME = 600;  // 10s
}  // namespace Sleep

}  // namespace Cfg