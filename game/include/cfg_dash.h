#pragma once

// -----------------------------------------------------------------------------
// Cfg::Dash
//
// Dash tuning values. Lives in its own dependency-free header (only
// built-in types, no Butano) so the host unit tests under tests/host/ can
// check the constants' invariants; cfg.h includes it, so every existing
// `Cfg::Dash::...` reference keeps working unchanged.
// -----------------------------------------------------------------------------

namespace Cfg {

namespace Dash {
// Horizontal burst on B (Hollow Knight style): gravity is suspended and the
// duck travels SPEED px/frame for DURATION_FRAMES, i.e. a flat 32px on the
// ground. In the air it is usable once per air time (refilled on landing);
// on the ground only the cooldown limits it. Dash only ever ADDS reach -
// the climb envelope (~22px) is unchanged, horizontal reach grows from ~54px
// (jump) to roughly 100px (jump + dash; same-height edge-to-edge gaps up to
// ~61px with a jump, ~103px with jump + dash, measured with
// tools/Check_level_reachability.py) - so no existing layout becomes
// unreachable. SPEED must stay below Player's vel_max (5) because
// PhysicsBody::move_velocity() clamps by magnitude.
inline constexpr int SPEED = 4;
inline constexpr int DURATION_FRAMES = 8;
inline constexpr int COOLDOWN_FRAMES = 20;  // ~0.33s between dashes
// Index into WorldIndex::WORLDS of the first world with dash enabled
// (2 = World 3, Forest). Earlier levels were authored without it and keep
// their design; set to 0 to enable it everywhere.
inline constexpr int UNLOCK_WORLD = 2;
}  // namespace Dash

}  // namespace Cfg
