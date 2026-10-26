#pragma once

// -----------------------------------------------------------------------------
// Cfg::Pogo
//
// The pogo strike (Hollow Knight's down slash) on R in the air. Traps stay
// unbreakable: a strike that meets one below the duck bounces it up instead
// of killing it. Dependency free like cfg_dash.h; cfg.h includes it.
// -----------------------------------------------------------------------------

namespace Cfg {

namespace Pogo {
// How long a strike reaches below the duck, then how long until the next.
inline constexpr int STRIKE_FRAMES = 10;
inline constexpr int COOLDOWN_FRAMES = 8;
// The strike's box: REACH px below the body's bottom, WIDTH px wide,
// centered under the duck. REACH must exceed Player::MAX_FALL_SPEED so a
// falling duck meets the trap with the strike before its body touches it.
inline constexpr int REACH = 12;
inline constexpr int WIDTH = 16;
// The bounce: up at Player::JUMP_SPEED, never cut short by letting go of A.
inline constexpr int BOUNCE_SPEED = -3;
// Index into WorldIndex::WORLDS of the first world with the pogo
// (3 = World 4). Earlier levels were authored without it.
inline constexpr int UNLOCK_WORLD = 3;
}  // namespace Pogo

}  // namespace Cfg
