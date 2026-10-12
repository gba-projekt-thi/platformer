#!/usr/bin/env python3
"""
Check_checkpoints.py

Statically validates the checkpoints (LevelData::checkpoints, CheckpointData)
of game/include/level/levels_world*.h. A checkpoint's (x, y) is the center of
the flag sprite AND the respawn position of the duck, so a bad placement means
a bad respawn: the duck falls into the pit, lands inside a platform or a trap,
or the flag floats where it can never be reached.

Errors (exit code 1):
  - more checkpoints than Cfg::Level::Limits::CHECKPOINTS (read from cfg.h)
  - outside the world bounds
  - the flag does not stand on a platform: its bottom (y + 8) must be at most
    4px above or 0px into the top of a platform that spans its x, and that
    platform must not be a switched one (it can vanish)
  - the 8x8 respawn box overlaps a platform, a trap's start box, a PATH trap's
    route node or the box of an AMBUSH trap anywhere along its lunge
  - the supporting platform cannot be reached from the spawn (checked with the
    jump + dash movement model of Check_level_reachability.py, over every lever
    configuration)

Warnings:
  - the respawn lies inside an AMBUSH trap's trigger range (it would lunge at
    once, even if it cannot hit)
  - checkpoints are not in left-to-right order, or two are closer than 24px
  - the supporting platform is only reachable with the dash

Usage (from the repository root):
    python3 tools/Check_checkpoints.py [file ...]

Pure text parsing + the movement model; no C++ compiler involved.
"""

import glob
import importlib.util
import re
import sys
from pathlib import Path

_here = Path(__file__).resolve().parent


def _load(name, filename):
    spec = importlib.util.spec_from_file_location(name, _here / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


pv = _load("preview_level", "Preview_level.py")
sim = pv.sim

DEFAULT_MAX = 4
FLAG_HALF = 8  # the flag sprite is 16x16, its center is the respawn point
DUCK = 8
MAX_FLOAT = 4  # px the flag may hover above the platform top
MIN_SPACING = 24


def max_checkpoints(search_root=None):
    root = Path(search_root) if search_root else Path(".")
    for path in sorted(root.glob("**/cfg.h")):
        match = re.search(r"CHECKPOINTS\s*=\s*(\d+)", path.read_text())
        if match:
            return int(match.group(1))
    return DEFAULT_MAX


def _trap_boxes(trap):
    """Boxes (cx, cy, w, h) the trap occupies at its start (and, for PATH
    traps, at every route node; for AMBUSH traps along the lunge)."""
    x, y = pv.trap_position(trap)
    w, h = trap["width"], trap["height"]
    if trap["type"] == "PATH" and trap["path"]:
        return [
            (trap["x"] + px + trap["offset_x"], trap["y"] + py + trap["offset_y"], w, h)
            for px, py in trap["path"]
        ]
    if trap["type"] == "AMBUSH":
        travel = trap["ambush_speed"] * trap["ambush_duration"]
        steps = max(1, int(abs(travel)))
        return [(x + travel * i / steps, y, w, h) for i in range(steps + 1)]
    return [(x, y, w, h)]


def check_level(level, limit=DEFAULT_MAX, reach=True):
    """Returns (errors, warnings) for one Level."""
    errors, warnings = [], []
    points = level.checkpoints
    if not points:
        return errors, warnings

    def err(text):
        errors.append(f"{level.name}: {text}")

    def warn(text):
        warnings.append(f"{level.name}: {text}")

    if len(points) > limit:
        err(f"{len(points)} checkpoints, at most {limit} allowed")

    half_w, half_h = level.world_w / 2, level.world_h / 2
    reached_plain = reached_dash = None
    if reach:
        sim.attach_lever_platforms(level.platforms, level.sim_triggers)
        reached_plain, _, _ = sim.reach_with_levers(
            level.platforms, level.sim_triggers, level.spawn, level.door_hit, False, False
        )
        reached_dash, _, _ = sim.reach_with_levers(
            level.platforms, level.sim_triggers, level.spawn, level.door_hit, True, False
        )

    for i, (x, y) in enumerate(points, 1):
        where = f"checkpoint {i} at ({x:g},{y:g})"
        if abs(x) > half_w or abs(y) > half_h:
            err(f"{where} is outside the world ({level.world_w:g}x{level.world_h:g})")

        bottom = y + FLAG_HALF
        support = None
        for p in level.platforms:
            if p.left <= x <= p.right and 0 <= p.top - bottom <= MAX_FLOAT:
                support = p
                break
        if support is None:
            err(
                f"{where}: the flag does not stand on a platform (its bottom y={bottom:g} "
                f"must be within {MAX_FLOAT}px above a platform top)"
            )
        elif support.trigger is not None:
            err(f"{where}: stands on platform {support.index}, which is switched by '{support.trigger}'")
        elif reach:
            if support.index not in reached_dash:
                err(f"{where}: platform {support.index} cannot be reached from the spawn")
            elif support.index not in reached_plain:
                warn(f"{where}: platform {support.index} is only reachable with the dash")

        for p in level.platforms:
            if sim.overlap(x, y, DUCK, DUCK, p.cx, p.cy, p.w, p.h):
                err(f"{where}: the respawn overlaps platform {p.index}")

        for trap in level.traps:
            for bx, by, bw, bh in _trap_boxes(trap):
                if sim.overlap(x, y, DUCK, DUCK, bx, by, bw, bh):
                    err(f"{where}: the respawn overlaps a {trap['type']} trap at ({bx:g},{by:g})")
                    break
            if trap["type"] == "AMBUSH":
                tx, _ = pv.trap_position(trap)
                if abs(x - tx) <= trap["ambush_range"]:
                    warn(
                        f"{where}: inside the ambush range ({trap['ambush_range']:g}px) of the "
                        f"trap at x={tx:g} - it lunges right after the respawn"
                    )

    for (ax, _), (bx, _) in zip(points, points[1:]):
        if bx < ax:
            warn("checkpoints are not in left-to-right order (fine for a vertical level)")
            break
    for (ax, ay), (bx, by) in zip(points, points[1:]):
        if abs(bx - ax) < MIN_SPACING and abs(by - ay) < MIN_SPACING:
            warn(f"checkpoints at ({ax:g},{ay:g}) and ({bx:g},{by:g}) are closer than {MIN_SPACING}px")
    return errors, warnings


def main(argv):
    paths = [Path(p) for p in argv] or sorted(
        Path(p) for p in glob.glob("**/levels_world*.h", recursive=True)
    )
    if not paths:
        print("No levels_world*.h files found - nothing to check.", file=sys.stderr)
        return 1
    limit = max_checkpoints()
    all_errors, all_warnings, checked = [], [], 0
    for path in paths:
        for level in pv.load_level_file(path).values():
            if level.checkpoints:
                checked += 1
            errors, warnings = check_level(level, limit)
            all_errors.extend(errors)
            all_warnings.extend(warnings)
    for w in all_warnings:
        print(f"WARN  {w}")
    for e in all_errors:
        print(f"ERROR {e}", file=sys.stderr)
    print(
        f"\nChecked {checked} level(s) with checkpoints in {len(paths)} file(s): "
        f"{len(all_errors)} error(s), {len(all_warnings)} warning(s)."
    )
    return 1 if all_errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
