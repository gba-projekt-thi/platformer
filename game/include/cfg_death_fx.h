#pragma once

// -----------------------------------------------------------------------------
// Cfg::DeathFx
//
// Tuning of the death feedback (see death_fx.h). Lives in its own
// dependency-free header (only built-in types, no Butano) so the host unit
// tests under tests/host/ can check the constants' invariants; cfg.h includes
// it.
// -----------------------------------------------------------------------------

namespace Cfg {

namespace DeathFx {
// World frozen on the death frame and the frames after it, so the player sees
// what killed the duck (~0.1s). Must be at least 1.
inline constexpr int HITSTOP_FRAMES = 6;
// Screen shake: starts at SHAKE_AMPLITUDE px and fades out. It outlasts the
// freeze, so the picture is still settling after the duck is back.
inline constexpr int SHAKE_FRAMES = 14;
inline constexpr int SHAKE_AMPLITUDE = 3;
// After the freeze the duck flickers for a moment at the respawn point so the
// eye finds it again. Purely visual: it gives NO invulnerability.
inline constexpr int RESPAWN_BLINK_FRAMES = 18;
}  // namespace DeathFx

}  // namespace Cfg
