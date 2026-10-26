#!/usr/bin/env python3
"""
Check_level_reachability.py

Static jump/dash reachability check for the level data in
game/include/level/levels_world*.h, using a small re-implementation of the
duck's per-frame movement (PlayerLocomotion::update() + PhysicsBody::move(),
same constants as Cfg::Player / Cfg::Dash).

What it answers, per level:
  - Which platforms can the duck reach from its spawn, WITHOUT dash and WITH
    dash? (platform-to-platform graph; every start/jump/dash timing from a
    small policy family is simulated)
  - Can it reach the door in each case?
  - For levels with switched platforms (PlatformData::trigger_name): the same
    question over every lever configuration the duck can set from the
    platforms it has already reached (a lever on a reached platform can be
    flipped freely).

Two start assumptions bracket real play:
  - conservative: the duck jumps with its center exactly on the platform's
    edge (no coyote time). If a platform is reachable here, a normal player
    can do it.
  - optimistic: the duck has already run ~11px past the edge (coyote time,
    full speed). If a platform is NOT reachable here, no player can do it, so
    a gap that is meant to require the dash must fail this check without dash.

Traps are ignored (this checks geometry, not hazard timing).

Usage:
    python3 tools/Check_level_reachability.py [--dash-from-index N] FILE LEVEL...

Exit code 0 if every requested level's door is reachable conservatively with
all abilities it is allowed to use; 1 otherwise. The module is also imported
by tools/test_check_level_reachability.py.
"""

import importlib.util
import re
import sys
from pathlib import Path

_here = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location(
    "check_trigger_references", _here / "Check_trigger_references.py"
)
_ctr = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_ctr)

# ---- constants mirrored from cfg.h / cfg_dash.h ---------------------------
ACC = 0.38
MAXS = 2.0
JUMP = -3.0
G = 0.22
MAXFALL = 3.0
DECEL = 0.3
COYOTE = 4
DASH_SPEED = 4.0
DASH_FRAMES = 8
BW = 8.0
BH = 8.0
DEATH_Y = 100.0
MAX_FRAMES = 160


class Rect:
    __slots__ = (
        "cx", "cy", "w", "h", "index", "trigger", "active_when_on",
        "crumble", "respawn",
    )

    def __init__(
        self, cx, cy, w, h, index=-1, trigger=None, active_when_on=True,
        crumble=0, respawn=0,
    ):
        self.cx, self.cy, self.w, self.h = cx, cy, w, h
        self.index = index
        self.trigger = trigger
        self.active_when_on = active_when_on
        # PlatformData::crumble_frames / crumble_respawn_frames. The check
        # treats a crumbling platform as solid: it holds the duck long enough
        # to jump off, so reachability is the same as for a plain one.
        self.crumble = crumble
        self.respawn = respawn

    @property
    def left(self):
        return self.cx - self.w / 2

    @property
    def right(self):
        return self.cx + self.w / 2

    @property
    def top(self):
        return self.cy - self.h / 2


def overlap(ax, ay, aw, ah, bx, by, bw, bh):
    return (
        ax - aw / 2 < bx + bw / 2
        and ax + aw / 2 > bx - bw / 2
        and ay - ah / 2 < by + bh / 2
        and ay + ah / 2 > by - bh / 2
    )


def _hits(plats, x, y, w, h):
    for p in plats:
        if overlap(x, y, w, h, p.cx, p.cy, p.w, p.h):
            return p
    return None


def simulate(
    plats,
    door,
    x0,
    y0,
    vx0,
    d,
    jump,
    hold,
    dash_t,
    start_plat,
    coyote0,
    release=MAX_FRAMES,
    trace=None,
):
    """Runs one policy. Returns (landed_platform_or_None, door_touched).

    `trace`, when a list, receives the duck's (x, y) center after every frame
    (used by tools/Preview_level.py to draw the jump arcs)."""
    x, y, vx, vy = x0, y0, vx0, 0.0
    if trace is not None:
        trace.append((x, y))
    on_ground = True
    coyote = coyote0
    dashing = False
    dash_timer = 0
    dash_used = False
    airborne = False
    door_touched = False
    for f in range(MAX_FRAMES):
        a_held = jump and f < hold
        a_pressed = jump and f == 0
        ended_this_frame = False
        if dash_t is not None and f == dash_t and not dash_used:
            dashing, dash_timer, dash_used = True, DASH_FRAMES, True
        moved_by_dash = False
        if dashing:
            if dash_timer == 0:
                dashing = False
                vx, vy = d * MAXS, 0.0
                ended_this_frame = True
            else:
                vx, vy = d * DASH_SPEED, 0.0
                dash_timer -= 1
                moved_by_dash = True
        if not moved_by_dash:
            # normal path (also the frame a dash ends)
            if f < release:
                vx = vx + d * ACC
                if abs(vx) > MAXS:
                    vx = d * MAXS
            elif vx > 0:  # direction released: brake toward zero
                vx = max(0.0, vx - DECEL)
            elif vx < 0:
                vx = min(0.0, vx + DECEL)
            vy += G
            if not a_held and vy < 0:
                vy *= 0.5
            if vy > MAXFALL:
                vy = MAXFALL
        # ground state (probe 1px below, width-2)
        gp = _hits(plats, x, y + BH / 2 + 1, BW - 2, 1)
        if gp is not None:
            on_ground, coyote = True, COYOTE
        elif coyote > 0:
            coyote -= 1
            on_ground = True
        else:
            on_ground = False
        if not moved_by_dash and a_pressed and on_ground:
            vy = JUMP
            on_ground = False
            coyote = 0
        # move: X then Y, blocked -> velocity zeroed
        nx = x + vx
        if _hits(plats, nx, y, BW, BH) is None:
            x = nx
        else:
            vx = 0.0
        ny = y + vy
        if _hits(plats, x, ny, BW, BH) is None:
            y = ny
        else:
            vy = 0.0
        if trace is not None:
            trace.append((x, y))
        if door is not None and overlap(x, y, BW, BH, door[0], door[1], 2, 2):
            door_touched = True
        if y > DEATH_Y:
            return None, door_touched
        gp = _hits(plats, x, y + BH / 2 + 1, BW - 2, 1)
        if gp is not None:
            if gp is not start_plat or airborne:
                if vy >= 0 and f >= 1:
                    return gp, door_touched
        if y < start_plat.top - BH / 2 - 1 or vy != 0:
            airborne = True
    return None, door_touched


def policies(allow_dash):
    dash_ts = [None]
    if allow_dash:
        dash_ts += list(range(0, 40, 5))
    for d in (1, -1):
        for jump, hold in ((True, 99), (True, 6), (False, 0)):
            for release in (MAX_FRAMES, 18, 12):
                if not jump and release != MAX_FRAMES:
                    continue
                for dash_t in dash_ts:
                    yield d, jump, hold, dash_t, release


START_OFFSETS = (0.0, 10.0, 20.0, 30.0)  # px back from the edge (conservative)


def landings(active, start, door, allow_dash, optimistic, traces=None):
    """Platforms (and door contact) reachable in ONE action from `start`.

    `traces`, when a dict, collects one jump trajectory per
    (start index, landed index, dash scheduled) - the shortest one found."""
    found = set()
    door_hit = False
    y0 = start.top - BH / 2
    for d, jump, hold, dash_t, release in policies(allow_dash):
        edge = start.right if d == 1 else start.left
        starts = []
        for k in START_OFFSETS:
            x0 = edge - d * k
            if start.left <= x0 <= start.right:
                # a run-up needs room behind the start point
                runway = (x0 - start.left) if d == 1 else (start.right - x0)
                starts.append((x0, d * MAXS if runway >= 12 else 0.0))
        if optimistic:
            # superset of the conservative starts, plus a duck that already
            # ran ~11px past the edge (coyote time at full speed)
            starts.append((edge + d * 11.0, d * MAXS))
        for x0, vx0 in starts:
            trace = [] if traces is not None else None
            landed, touched = simulate(
                active,
                door,
                x0,
                y0,
                vx0,
                d,
                jump,
                hold,
                dash_t,
                start,
                COYOTE,
                release,
                trace,
            )
            door_hit |= touched
            if landed is not None and landed is not start:
                found.add(landed)
                if traces is not None:
                    key = (start.index, landed.index, dash_t is not None)
                    if key not in traces or len(trace) < len(traces[key]):
                        traces[key] = trace
    return found, door_hit


def spawn_platform(active, spawn):
    x, y = spawn
    vy = 0.0
    for _ in range(300):
        vy = min(vy + G, MAXFALL)
        if _hits(active, x, y + vy, BW, BH) is None:
            y += vy
        else:
            return _hits(active, x, y + vy + 0.5, BW, BH) or _hits(
                active, x, y + BH / 2 + 1, BW - 2, 1
            )
        if y > DEATH_Y:
            return None
    return None


def reach(active, spawn, door, allow_dash, optimistic):
    """Returns (reached platforms, door reachable) for a fixed platform set."""
    first = spawn_platform(active, spawn)
    if first is None:
        return set(), False
    reached = {first}
    frontier = [first]
    door_ok = False
    while frontier:
        cur = frontier.pop()
        found, touched = landings(active, cur, door, allow_dash, optimistic)
        door_ok |= touched
        for p in found:
            if p not in reached:
                reached.add(p)
                frontier.append(p)
    # standing on a reached platform next to the door also counts
    if door is not None:
        for p in reached:
            if overlap(
                _clamp(door[0], p.left, p.right),
                p.top - BH / 2,
                BW,
                BH,
                door[0],
                door[1],
                2,
                2,
            ):
                door_ok = True
    return reached, door_ok


def _clamp(v, lo, hi):
    return max(lo, min(hi, v))


def reach_with_levers(platforms, triggers, spawn, door, allow_dash, optimistic):
    """Fixpoint over lever configurations. `triggers` maps name -> dict with
    default_on and the Rect of the platform the lever stands on (or None).
    Returns (reached_ids, door_ok, configs_explored)."""
    names = sorted(triggers)

    def active_for(config):
        out = []
        for p in platforms:
            if p.trigger is None:
                out.append(p)
            else:
                on = config[p.trigger]
                if on == p.active_when_on:
                    out.append(p)
        return out

    start = tuple((n, triggers[n]["default_on"]) for n in names)
    seen = {start}
    todo = [start]
    all_reached = set()
    door_ok = False
    while todo:
        cfg = dict(todo.pop())
        active = active_for(cfg)
        reached, ok = reach(active, spawn, door, allow_dash, optimistic)
        door_ok |= ok
        all_reached |= {p.index for p in reached}
        for n in names:
            holder = triggers[n]["platform"]
            if holder is not None and holder in reached:
                flipped = dict(cfg)
                flipped[n] = not cfg[n]
                key = tuple((k, flipped[k]) for k in names)
                if key not in seen:
                    seen.add(key)
                    todo.append(key)
    return all_reached, door_ok, len(seen)


# ---- level data parsing -----------------------------------------------------
def _num(s):
    return float(s.strip())


def _fields_of(braced):
    inner = braced.strip()
    if inner.startswith("{"):
        inner = inner[1:-1]
    return _ctr.top_level_fields(inner)


def load_levels(path):
    content = _ctr.strip_comments(Path(path).read_text())
    plat_arrays = _ctr.extract_named_arrays(content, "PlatformData")
    trig_arrays = _ctr.extract_named_arrays(content, "TriggerData")
    levels = {}
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        name = m.group(1)
        ob = m.end()
        if content[ob] != "{":
            continue
        cb = _ctr.find_balanced_block(content, ob)
        f = _ctr.top_level_fields(content[ob + 1 : cb])
        platforms = []
        if f[0] != "nullptr":
            for i, lit in enumerate(_ctr.extract_top_level_literals(plat_arrays[f[0]])):
                pf = _ctr.top_level_fields(lit)
                x, y, w, h, ox, oy = (_num(v) for v in pf[:6])
                trig = pf[8].strip('"') if len(pf) > 8 and pf[8].startswith('"') else None
                awo = (pf[9].strip() == "true") if len(pf) > 9 else True
                crumble = int(_num(pf[10])) if len(pf) > 10 else 0
                respawn = int(_num(pf[11])) if len(pf) > 11 else 0
                platforms.append(Rect(x + ox, y + oy, w, h, i, trig, awo, crumble, respawn))
        triggers = {}
        if f[2] != "nullptr":
            for lit in _ctr.extract_top_level_literals(trig_arrays[f[2]]):
                tf = _ctr.top_level_fields(lit)
                if len(tf) > 5 and tf[5].startswith('"'):
                    tx, ty, tw, th = (_num(v) for v in tf[:4])
                    triggers[tf[5].strip('"')] = {
                        "default_on": tf[4].strip() == "true",
                        "rect": (tx, ty, tw, th),
                    }
        spawn = tuple(_num(v) for v in _fields_of(f[8])[:2])
        door = _fields_of(f[9])
        door_pos = (_num(door[0]), _num(door[1]) + 6.0)
        levels[name] = (platforms, triggers, spawn, door_pos)
    return levels


def attach_lever_platforms(platforms, triggers):
    """A lever 'stands on' the platform whose top meets the trigger's bottom."""
    for name, t in triggers.items():
        tx, ty, tw, th = t["rect"]
        holder = None
        for p in platforms:
            if p.trigger is not None:
                continue
            if abs((ty + th / 2) - p.top) <= 3 and tx + tw / 2 > p.left and tx - tw / 2 < p.right:
                holder = p
        t["platform"] = holder


def check_level(platforms, triggers, spawn, door, dash):
    attach_lever_platforms(platforms, triggers)
    out = {}
    for opt in (False, True):
        r, ok, n = reach_with_levers(platforms, triggers, spawn, door, dash, opt)
        out[("optimistic" if opt else "conservative", "dash" if dash else "no dash")] = (r, ok, n)
    return out


def main(argv):
    args = argv[:]
    if len(args) < 2:
        print(__doc__)
        return 2
    path, names = args[0], args[1:]
    levels = load_levels(path)
    bad = False
    for name in names:
        platforms, triggers, spawn, door = levels[name]
        print(f"== {name}: {len(platforms)} platforms, {len(triggers)} triggers")
        for dash in (False, True):
            res = check_level(platforms, triggers, spawn, door, dash)
            for key, (r, ok, n) in res.items():
                print(
                    f"  {key[0]:12s} {key[1]:8s} reached {len(r):2d}/{len(platforms)} "
                    f"door={'yes' if ok else 'NO '} configs={n}"
                )
        cons_dash = check_level(platforms, triggers, spawn, door, True)[("conservative", "dash")]
        if not cons_dash[1]:
            bad = True
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
