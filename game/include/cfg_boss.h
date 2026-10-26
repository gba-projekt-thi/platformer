#pragma once

// -----------------------------------------------------------------------------
// Cfg::Boss
//
// Boss fights (TrapType::BOSS, boss_logic.h). Dependency free like
// cfg_dash.h; cfg.h includes it.
// -----------------------------------------------------------------------------

namespace Cfg {

namespace Boss {
// After a hit the boss blinks and takes no further hit for this long, so a
// single strike never counts twice.
inline constexpr int INVULNERABLE_FRAMES = 45;
inline constexpr int BLINK_FRAMES = 4;  // on / off period while blinking
// A boss has at most this many hits (and phases); phase n switches on the
// trigger "<arena trigger>_p<n>".
inline constexpr int MAX_PHASES = 6;
}  // namespace Boss

}  // namespace Cfg
