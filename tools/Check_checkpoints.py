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
    platform must be neither a switched one nor a crumbling one (it can vanish)
  - the 8x8 respawn box overlaps a platform, a trap's start box or anywhere on
    a trap's path: the whole route of a PATH trap (the segments between its
    nodes, closing back to the first), the track a MOVING trap takes once its
    trigger fires, the stroke of a CYCLE trap (from its resting place to full
    extension), the line a CHASE trap sweeps to the right, or the lunge of
    an AMBUSH trap
  - the supporting platform cannot be reached from the spawn (checked with the
    jump + dash movement model of Check_level_reachability.py, over every lever
    configuration and, with Level Devil death windows, on every attempt)
  - the supporting platform has a death window (from_death / until_death): the
    flag would float on the attempts it is not there

  - the level spawn (every level, with or without checkpoints) or the duck's
    fall from it to the floor overlaps a platform or a trap on some attempt:
    checked once per Level Devil death window layout (death_breakpoints()),
    so a `from_death: 3` trap on the spawn is caught. Counted are a trap's
    start box and, for a trap that moves on its own from the first frame
    (PATH / CYCLE / MOVING without a trigger) or lunges (AMBUSH), its whole
    path; a trap gated by a trigger is assumed to start once the duck has left
    the spawn, and a CHASE trap hunts the duck by design

Warnings:
  - the respawn lies inside an AMBUSH trap's trigger range (it would lunge at
    once, even if it cannot hit)
  - checkpoints are not in left-to-right order, or two are closer than 24px
  - the supporting platform is only reachable with the dash, in a level that
    does not have the dash (Cfg::Dash::UNLOCK_WORLD)

Usage (from the repository root):
    python3 tools/Check_checkpoints.py [file ...]

Pure text parsing + the movement model; no C++ compiler involved.
"""

import importlib.util
import math
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

DEFAULT_MAX = 1
FLAG_HALF = 8  # the flag sprite is 16x16, its center is the respawn point
DUCK = 8
MAX_FLOAT = 4  # px the flag may hover above the platform top
MIN_SPACING = 24
SWEEP_STEP = 2.0  # px between the boxes sampled along a trap path
MOVING_FRAMES = 600  # how long a MOVING trap is followed (it stops at the world edge)


def max_checkpoints(search_root=None):
    path = pv._ctr.find_game_file(search_root or ".", "cfg.h")
    match = path and re.search(r"CHECKPOINTS\s*=\s*(\d+)", path.read_text())
    return int(match.group(1)) if match else DEFAULT_MAX


def _segment_boxes(a, b, w, h):
    """Boxes along the straight line a -> b, at most SWEEP_STEP px apart (the
    traps are bigger than that, so nothing slips through)."""
    (ax, ay), (bx, by) = a, b
    steps = max(1, int(math.ceil(max(abs(bx - ax), abs(by - ay)) / SWEEP_STEP)))
    return [(ax + (bx - ax) * i / steps, ay + (by - ay) * i / steps, w, h) for i in range(steps + 1)]


def _trap_boxes(level, trap):
    """Boxes (cx, cy, w, h) a trap can occupy: at its start and everywhere on
    its path - the whole route of a PATH trap (also the segment that closes the
    loop), the track of a MOVING trap until it leaves the world, the line a
    CHASE trap sweeps to the right, and the lunge of an AMBUSH trap."""
    kind = trap["type"]
    x, y = pv.trap_position(trap)
    w, h = trap["width"], trap["height"]
    if kind == "PATH" and trap["path"]:
        route = [
            (trap["x"] + px + trap["offset_x"], trap["y"] + py + trap["offset_y"])
            for px, py in trap["path"]
        ]
        boxes = []
        for i, node in enumerate(route):
            boxes += _segment_boxes(node, route[(i + 1) % len(route)], w, h)
        return boxes
    if kind == "AMBUSH":
        travel = trap["ambush_speed"] * trap["ambush_duration"]
        return _segment_boxes((x, y), (x + travel, y), w, h)
    if kind == "CYCLE":
        # From its resting place out to full extension and back.
        return _segment_boxes((x, y), (x + trap["cycle_dx"], y + trap["cycle_dy"]), w, h)
    if kind == "MOVING":
        boxes = []
        for px, py in pv.moving_track(trap, MOVING_FRAMES):
            if abs(px) > level.world_w / 2 + pv.TRACK_MARGIN or abs(py) > (
                level.world_h / 2 + pv.TRACK_MARGIN
            ):
                break
            boxes.append((px, py, w, h))
        return boxes or [(x, y, w, h)]
    if kind == "CHASE":
        # ChaserTrap only ever moves right (it never retreats), along its row.
        # chase_limit > 0 stops it that far right of its start.
        end = max(x, level.world_w / 2 + w)
        if trap.get("chase_limit"):
            end = min(end, x + trap["chase_limit"])
        return _segment_boxes((x, y), (end, y), w, h)
    return [(x, y, w, h)]


def _after(deaths):
    return f" after {deaths} deaths" if deaths else ""


def _fall_boxes(active, spawn):
    """The duck's 8x8 box from the spawn down to where it lands on `active`
    (just the spawn when it lands nowhere): it falls before it can move."""
    x, y = spawn
    floor = sim.spawn_platform(active, spawn)
    if floor is None or floor.top - DUCK / 2 <= y:
        return [(x, y, DUCK, DUCK)]
    return _segment_boxes((x, y), (x, floor.top - DUCK / 2), DUCK, DUCK)


def _spawn_trap_boxes(level, trap):
    """Where a trap can hit a duck that has just spawned and waits there: its
    start box, and the whole path of a trap that moves on its own from the
    first frame (no trigger: a PATH, CYCLE or MOVING trap) or lunges at a
    duck in range (AMBUSH). A trap gated by a trigger is assumed to start
    when the duck has left the spawn, and a CHASE trap hunts the duck by
    design - for both only their start box counts."""
    if trap["type"] == "AMBUSH" or (trap["trigger"] is None and trap["type"] != "CHASE"):
        return _trap_boxes(level, trap)
    x, y = pv.trap_position(trap)
    return [(x, y, trap["width"], trap["height"])]


def check_spawn(level):
    """Errors for a level spawn that a platform or trap covers on some
    attempt (Level Devil death windows): a `from_death: 3` trap there kills
    the duck on every respawn from the third death on. Checks the spawn box
    and the fall to the floor once per death_breakpoints() layout."""
    errors = []
    where = f"{level.name}: the spawn"
    reported = set()
    for deaths in sim.death_breakpoints(level.platforms + [_Window(t) for t in level.traps]):
        # the platforms there on this attempt, switched ones as the level starts
        active = [
            p for p in level.platforms
            if sim.present(p, deaths) and (
                p.trigger is None
                or level.sim_triggers.get(p.trigger, {}).get("default_on", False) == p.active_when_on
            )
        ]
        fall = _fall_boxes(active, level.spawn)
        for p in level.platforms:
            if p.index in reported or not sim.present(p, deaths):
                continue
            if any(sim.overlap(x, y, w, h, p.cx, p.cy, p.w, p.h) for x, y, w, h in fall[:1]):
                reported.add(p.index)
                errors.append(f"{where} overlaps platform {p.index}{_after(deaths)}")
        for i, trap in enumerate(level.traps):
            if ("trap", i) in reported or not sim.present(_Window(trap), deaths):
                continue
            hit = next(
                ((bx, by) for bx, by, bw, bh in _spawn_trap_boxes(level, trap)
                 if any(sim.overlap(x, y, w, h, bx, by, bw, bh) for x, y, w, h in fall)),
                None,
            )
            if hit is not None:
                reported.add(("trap", i))
                what = "trap" if trap["type"] == "BASE" else "trap's path"
                errors.append(
                    f"{where} overlaps a {trap['type']} {what} at ({hit[0]:g},{hit[1]:g})"
                    f"{_after(deaths)}"
                )
    return errors


class _Window:
    """A trap dict's death window as sim.present() / death_breakpoints() read it."""

    def __init__(self, trap):
        self.from_death, self.until_death = trap["from_death"], trap["until_death"]


def check_level(level, limit=DEFAULT_MAX, reach=True, dash_granted=False):
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
    # Level Devil: {deaths: (reached plain, reached with dash)} for every set
    # of platforms the level can have (death_breakpoints()).
    reached = {}
    if reach:
        sim.attach_lever_platforms(level.platforms, level.sim_triggers)
        for deaths in sim.death_breakpoints(level.platforms):
            active = [p for p in level.platforms if sim.present(p, deaths)]
            reached[deaths] = tuple(
                sim.reach_with_levers(
                    active, level.sim_triggers, level.spawn, level.door_hit, dash, False
                )[0]
                for dash in (False, True)
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
        elif support.crumble:
            err(f"{where}: stands on platform {support.index}, which crumbles after {support.crumble} frames")
        elif support.from_death or support.until_death:
            last = f"{support.until_death - 1}" if support.until_death else "more"
            err(
                f"{where}: stands on platform {support.index}, which is only there after "
                f"{support.from_death} .. {last} deaths"
            )
        elif reach:
            for deaths, (plain, dash) in reached.items():
                after = f" after {deaths} deaths" if deaths else ""
                if support.index not in dash:
                    err(f"{where}: platform {support.index} cannot be reached from the spawn{after}")
                    break
                if support.index not in plain and not dash_granted:
                    warn(f"{where}: platform {support.index} is only reachable with the dash{after}")
                    break

        for p in level.platforms:
            if sim.overlap(x, y, DUCK, DUCK, p.cx, p.cy, p.w, p.h):
                err(f"{where}: the respawn overlaps platform {p.index}")

        for trap in level.traps:
            for bx, by, bw, bh in _trap_boxes(level, trap):
                if sim.overlap(x, y, DUCK, DUCK, bx, by, bw, bh):
                    what = "trap" if trap["type"] == "BASE" else "trap's path"
                    err(f"{where}: the respawn overlaps a {trap['type']} {what} at ({bx:g},{by:g})")
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
    paths = [Path(p) for p in argv] or pv._ctr.level_files()
    if not paths:
        print("No levels_world*.h files found - nothing to check.", file=sys.stderr)
        return 1
    limit = max_checkpoints()
    # the dash is on from Cfg::Dash::UNLOCK_WORLD onward (see campaign())
    dash_levels = {lv.name for lv in sim.campaign(_here.parent) if lv.dash}
    all_errors, all_warnings, checked = [], [], 0
    for path in paths:
        for level in pv.load_level_file(path).values():
            if level.checkpoints:
                checked += 1
            errors, warnings = check_level(level, limit, dash_granted=level.name in dash_levels)
            all_errors.extend(errors)
            all_errors.extend(check_spawn(level))
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
