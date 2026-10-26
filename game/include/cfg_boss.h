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
// A hit is felt: the world stands still for HITSTOP_FRAMES and the picture
// shakes for SHAKE_FRAMES (death_fx::Sequence, as a death does - shorter).
inline constexpr int HITSTOP_FRAMES = 4;
inline constexpr int SHAKE_FRAMES = 10;
// A boss has at most this many hits (and phases); phase n switches on the
// trigger "<arena trigger>_p<n>".
inline constexpr int MAX_PHASES = 6;
}  // namespace Boss

}  // namespace Cfg
