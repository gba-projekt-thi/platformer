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

Traps are ignored (this checks geometry, not hazard timing). Wall jumps
(from Cfg::WallJump::UNLOCK_WORLD on) are simulated frame by frame by Duck,
with a few climb policies (between two walls, up one wall).

Usage (from the repository root):
    python3 tools/Check_level_reachability.py --all
        Every level of main.cpp's levels[] in play order; the dash and wall
        jumps count only from their Cfg::...::UNLOCK_WORLD on, as in the game.
    python3 tools/Check_level_reachability.py FILE LEVEL...
        Detailed report (with and without dash, both start assumptions) for
        the named levels of one file.

Exit code 0 if every checked level's door is reachable conservatively (with
the dash where --all allows it, always with it for FILE LEVEL...); 1
otherwise. The module is also imported by tools/test_check_level_reachability.py.
"""

import importlib.util
import json
import re
import sys
from collections import namedtuple
from pathlib import Path

_here = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location(
    "check_trigger_references", _here / "Check_trigger_references.py"
)
_ctr = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_ctr)
_spec = importlib.util.spec_from_file_location(
    "check_world_index", _here / "Check_world_index.py"
)
_cwi = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_cwi)

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
EDGE_MARGIN = 10  # Cfg::Screen: the duck bounces back past world_width / 2 - this
MAX_FRAMES = 160
JUMP_BUFFER = 6
# Cfg::WallJump
SLIDE = 1.0
PUSH = 2.0
LOCK = 8
WALL_FRAMES = 400  # a climb up a shaft takes longer than one jump
# Cfg::Pogo
POGO_FRAMES = 10
POGO_COOLDOWN = 8
POGO_REACH = 12
POGO_WIDTH = 16
POGO_BOUNCE = -3.0


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


def resolve_axis(frm, delta, blocked):
    """PhysicsBody::move() on one axis (move_resolution.h): the whole move if
    it is free, else steps of at most 1 px up to the obstacle. Returns
    (position, blocked)."""
    if delta == 0 or not blocked(frm + delta):
        return frm + delta, False
    one = 1.0 if delta > 0 else -1.0
    pos, remaining = frm, delta
    while remaining != 0:
        step = one if abs(remaining) > 1.0 else remaining
        if blocked(pos + step):
            break
        pos += step
        remaining -= step
    return pos, True


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
                vy = 0.0  # letting go of A ends the rise (locomotion::Core)
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
        # move: X then Y up to any obstacle, blocked -> velocity zeroed
        x, blocked = resolve_axis(x, vx, lambda nx: _hits(plats, nx, y, BW, BH) is not None)
        if blocked:
            vx = 0.0
        y, blocked = resolve_axis(y, vy, lambda ny: _hits(plats, x, ny, BW, BH) is not None)
        if blocked:
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


def fx(value):
    """`value` as bn::fixed stores it (12 fraction bits, truncated)."""
    return int(value * 4096) / 4096


def fx_mul(a, b):
    """bn::fixed * bn::fixed: both reduced to 6 fraction bits first."""
    return int(int(a * 4096) / 64) * int(int(b * 4096) / 64) / 4096


def _probe(plats, x, y, w, h):
    return _hits(plats, x, y, w, h) is not None


# Braking in the game: DECELERATION * ground friction 1 as bn::fixed computes it.
BRAKE = fx_mul(fx(DECEL), 1)


class Duck:
    """One frame of locomotion::Core::step() and PhysicsBody::move(), button by
    button, with the jump buffer, the wall slide / wall jump and the pogo
    strike off `pogo_targets` (trap boxes; no dash and no world-edge bounce).
    Uses the constants as bn::fixed stores them (fx()), so a scripted run
    stays frame-exact with the ROM. Drives the wall-climb policies below and
    scripted replays."""

    def __init__(self, plats, x, y, vx=0.0, wall_jump=True, pogo_targets=()):
        self.plats = plats
        self.x, self.y, self.vx, self.vy = x, y, vx, 0.0
        self.wall_jump = wall_jump
        self.on_ground = True
        self.coyote = 0
        self.buffer = 0
        self.lock = 0
        self.wall_jumped = False
        self.pogo_targets = list(pogo_targets)
        self.pogo_frames = 0
        self.pogo_cooldown = 0
        self.pogo_rise = False
        self.pogo_bounced = False

    def wall(self):
        """-1 / 1 = a wall touches the left / right side, 0 = none."""
        if not self.wall_jump:
            return 0
        if _probe(self.plats, self.x - BW / 2 - 1, self.y, 1, BH - 2):
            return -1
        return 1 if _probe(self.plats, self.x + BW / 2 + 1, self.y, 1, BH - 2) else 0

    def _pogo(self, r_pressed):
        self.pogo_bounced = False
        if r_pressed and self.pogo_targets and not self.on_ground \
                and self.pogo_frames == 0 and self.pogo_cooldown == 0:
            self.pogo_frames = POGO_FRAMES
        elif self.pogo_cooldown > 0:
            self.pogo_cooldown -= 1
        if self.pogo_frames == 0:
            return
        below = self.y + BH / 2 + POGO_REACH / 2
        if _probe(self.pogo_targets, self.x, below, POGO_WIDTH, POGO_REACH):
            self.vy, self.pogo_rise, self.pogo_frames = POGO_BOUNCE, True, 0
            self.pogo_bounced = True
        else:
            self.pogo_frames -= 1
            if self.pogo_frames == 0:
                self.pogo_cooldown = POGO_COOLDOWN

    def step(self, left=False, right=False, a_pressed=False, a_held=True, r_pressed=False):
        if a_pressed:
            self.buffer = JUMP_BUFFER
        wall = self.wall()
        if self.lock > 0:
            self.lock -= 1
        elif left or right:
            d = -1 if left else 1
            self.vx = max(-MAXS, min(MAXS, self.vx + d * fx(ACC)))
        elif self.vx > 0:
            self.vx = max(0.0, self.vx - BRAKE)
        elif self.vx < 0:
            self.vx = min(0.0, self.vx + BRAKE)
        self.vy += fx(G)
        if self.vy >= 0:
            self.pogo_rise = False
        if not a_held and self.vy < 0 and not self.pogo_rise:
            self.vy = 0.0
        self.vy = min(self.vy, MAXFALL)
        toward = (wall < 0 and left) or (wall > 0 and right)
        if toward and not self.on_ground and self.vy > SLIDE:
            self.vy = SLIDE
        self._pogo(r_pressed)
        if _probe(self.plats, self.x, self.y + BH / 2 + 1, BW - 2, 1):
            self.on_ground, self.coyote, self.lock = True, COYOTE, 0
        elif self.coyote > 0:
            self.coyote -= 1
            self.on_ground = True
        else:
            self.on_ground = False
        self.wall_jumped = False
        if self.buffer > 0 and self.on_ground:
            self.vy, self.on_ground, self.buffer, self.coyote = JUMP, False, 0, 0
        elif self.buffer > 0 and wall != 0:
            self.vy, self.vx = JUMP, -wall * PUSH
            self.lock, self.buffer = LOCK, 0
            self.wall_jumped = True
        if self.vy < 0 and _probe(self.plats, self.x, self.y - BH / 2 - 1, BW - 2, 1):
            self.vy = 0.0
        if self.buffer > 0:
            self.buffer -= 1
        self.x, blocked = resolve_axis(self.x, self.vx, lambda nx: _probe(self.plats, nx, self.y, BW, BH))
        if blocked:
            self.vx = 0.0
        self.y, blocked = resolve_axis(self.y, self.vy, lambda ny: _probe(self.plats, self.x, ny, BW, BH))
        if blocked:
            self.vy = 0.0


def climb(plats, door, start, x0, vx0, d, mode, slide, jump):
    """A wall-jump policy from `start`: run toward d (jumping first if `jump`),
    and at every wall touched in the air slide `slide` frames, let go of A for
    a frame and jump off it - then hold toward the next wall ("chimney", the
    one the push leads to) or back toward the same one ("same"). Returns
    (landed platform or None, door touched), like simulate()."""
    duck = Duck(plats, x0, start.top - BH / 2, vx0)
    touching = 0
    airborne = False
    door_touched = False
    for f in range(WALL_FRAMES):
        at = duck.wall()
        touching = touching + 1 if (at == d and not duck.on_ground) else 0
        a_held = touching != slide + 1
        pressed = (jump and f == 0) or touching == slide + 2
        duck.step(left=d < 0, right=d > 0, a_pressed=pressed, a_held=a_held)
        if duck.wall_jumped:
            touching = 0
            if mode == "chimney":
                d = -d
        if door is not None and overlap(duck.x, duck.y, BW, BH, door[0], door[1], 2, 2):
            door_touched = True
        if duck.y > DEATH_Y:
            return None, door_touched
        ground = _hits(plats, duck.x, duck.y + BH / 2 + 1, BW - 2, 1)
        if ground is not None and (ground is not start or airborne) and duck.vy >= 0 and f >= 1:
            return ground, door_touched
        if duck.y < start.top - BH / 2 - 1 or duck.vy != 0:
            airborne = True
    return None, door_touched


def pogo_hop(plats, traps, door, start, x0, vx0, d, r_from, wall_jump):
    """A pogo policy from `start`: jump toward d holding A, and from frame
    `r_from` on press R every other frame, bouncing off any trap (`traps`)
    below. Returns (landed platform or None, door touched), like simulate().
    Touching a trap with the body is death."""
    duck = Duck(plats, x0, start.top - BH / 2, vx0, wall_jump=wall_jump, pogo_targets=traps)
    airborne = False
    door_touched = False
    bounces = 0
    for f in range(WALL_FRAMES):
        if bounces > 4:  # hopping in place on one trap leads nowhere
            return None, door_touched
        r = f >= r_from and (f - r_from) % 2 == 0
        duck.step(left=d < 0, right=d > 0, a_pressed=f == 0, a_held=True, r_pressed=r)
        bounces += duck.pogo_bounced
        if any(overlap(duck.x, duck.y, BW, BH, t.cx, t.cy, t.w, t.h) for t in traps):
            return None, door_touched
        if door is not None and overlap(duck.x, duck.y, BW, BH, door[0], door[1], 2, 2):
            door_touched = True
        if duck.y > DEATH_Y:
            return None, door_touched
        ground = _hits(plats, duck.x, duck.y + BH / 2 + 1, BW - 2, 1)
        if ground is not None and (ground is not start or airborne) and duck.vy >= 0 and f >= 1:
            return ground, door_touched
        if duck.y < start.top - BH / 2 - 1 or duck.vy != 0:
            airborne = True
    return None, door_touched


POGO_STARTS = (0, 8, 14, 20)


WALL_POLICIES = [
    (d, mode, slide, jump)
    for d in (1, -1) for mode in ("chimney", "same") for slide in (0, 8) for jump in (True, False)
]


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


def landings(active, start, door, allow_dash, optimistic, traces=None, allow_wall=False,
             pogo_targets=()):
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
    # only traps a jump from here can reach (about 60 px past either edge)
    near = [t for t in pogo_targets if start.left - 80 < t.cx < start.right + 80]
    if near:
        for d in (1, -1):
            edge = start.right if d == 1 else start.left
            for k in START_OFFSETS:
                x0 = edge - d * k
                if not start.left <= x0 <= start.right:
                    continue
                runway = (x0 - start.left) if d == 1 else (start.right - x0)
                vx0 = d * MAXS if runway >= 12 else 0.0
                for r_from in POGO_STARTS:
                    landed, touched = pogo_hop(
                        active, near, door, start, x0, vx0, d, r_from, allow_wall)
                    door_hit |= touched
                    if landed is not None and landed is not start:
                        found.add(landed)
    if allow_wall:
        for d, mode, slide, jump in WALL_POLICIES:
            edge = start.right if d == 1 else start.left
            for k in START_OFFSETS:
                x0 = edge - d * k
                if not start.left <= x0 <= start.right:
                    continue
                landed, touched = climb(active, door, start, x0, 0.0, d, mode, slide, jump)
                door_hit |= touched
                if landed is not None and landed is not start:
                    found.add(landed)
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


def reach(active, spawn, door, allow_dash, optimistic, allow_wall=False, pogo_targets=()):
    """Returns (reached platforms, door reachable) for a fixed platform set."""
    first = spawn_platform(active, spawn)
    if first is None:
        return set(), False
    reached = {first}
    frontier = [first]
    door_ok = False
    while frontier:
        cur = frontier.pop()
        found, touched = landings(active, cur, door, allow_dash, optimistic, allow_wall=allow_wall,
                                  pogo_targets=pogo_targets)
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


def reach_with_levers(platforms, triggers, spawn, door, allow_dash, optimistic, allow_wall=False,
                      pogo_targets=()):
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

    # A boss's "<arena>_won" trigger (BossTrap) counts as on: the check
    # assumes the fight is won (the emulator replays prove that).
    start = tuple((n, triggers[n]["default_on"] or n.endswith("_won")) for n in names)
    seen = {start}
    todo = [start]
    all_reached = set()
    door_ok = False
    while todo:
        cfg = dict(todo.pop())
        active = active_for(cfg)
        reached, ok = reach(active, spawn, door, allow_dash, optimistic, allow_wall, pogo_targets)
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
    flee_paths = {
        m.group(1): [(_num(x), _num(y)) for x, y in re.findall(r"\{\s*([-\d.]+)\s*,\s*([-\d.]+)\s*\}", m.group(2))]
        for m in re.finditer(r"constexpr bn::fixed_point (\w+)\[\]\s*=\s*\{(.*?)\};", content, re.S)
    }
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
                # a running platform (flee_path) counts where it ends up
                if len(pf) > 12 and pf[12].strip() in flee_paths:
                    dx, dy = flee_paths[pf[12].strip()][-1]
                    x, y = x + dx, y + dy
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
        # Only a trigger some platform is bound to can change what is
        # reachable; the rest (trap cues, delayed trap chains) are no levers.
        switching = {p.trigger for p in platforms if p.trigger is not None}
        triggers = {n: t for n, t in triggers.items() if n in switching}
        spawn =tuple(_num(v) for v in _fields_of(f[8])[:2])
        door = _fields_of(f[9])
        door_pos = (_num(door[0]), _num(door[1]) + 6.0)
        # A running door (DoorData::flee_path) has to be reached where it
        # ends up: its last stop.
        if len(door) > 6 and door[6].strip() in flee_paths:
            dx, dy = flee_paths[door[6].strip()][-1]
            door_pos = (door_pos[0] + dx, door_pos[1] + dy)
        levels[name] = (platforms, triggers, spawn, door_pos)
    return levels


def load_static_traps(path):
    """{level: [Rect]} of the BASE traps (they never move): what a pogo strike
    can bounce off for sure."""
    content = _ctr.strip_comments(Path(path).read_text())
    trap_arrays = _ctr.extract_named_arrays(content, "TrapData")
    out = {}
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        ob = m.end()
        if content[ob] != "{":
            continue
        cb = _ctr.find_balanced_block(content, ob)
        f = _ctr.top_level_fields(content[ob + 1: cb])
        traps = []
        if f[4] != "nullptr":
            for i, lit in enumerate(_ctr.extract_top_level_literals(trap_arrays[f[4]])):
                tf = _ctr.top_level_fields(lit)
                if tf[0].strip() == "TrapType::BASE":
                    x, y, w, h, ox, oy = (_num(v) for v in tf[1:7])
                    traps.append(Rect(x + ox, y + oy, w, h, i))
        out[m.group(1)] = traps
    return out


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


def check_level(platforms, triggers, spawn, door, dash, wall=False):
    attach_lever_platforms(platforms, triggers)
    out = {}
    for opt in (False, True):
        r, ok, n = reach_with_levers(platforms, triggers, spawn, door, dash, opt, wall)
        out[("optimistic" if opt else "conservative", "dash" if dash else "no dash")] = (r, ok, n)
    return out


def dash_allowed(level_index, worlds, unlock_world):
    """Mirrors LevelManager::_load_player_spawn(): the dash is on from the first
    level of WorldIndex::WORLDS[Cfg::Dash::UNLOCK_WORLD] onward."""
    return level_index >= worlds[unlock_world][1]


CampaignLevel = namedtuple("CampaignLevel", "index name path dash wall pogo")


def campaign(root):
    """Every level of the game in play order (main.cpp's levels[]), with the
    file that defines it and whether the dash, wall jumps and the pogo are
    available there (the pogo also in every level with a boss)."""
    root = Path(root)
    order = _cwi.parse_levels((root / "game/src/main.cpp").read_text())
    worlds = _cwi.parse_worlds((root / "game/include/world_index.h").read_text())
    def unlock(header):
        return int(re.search(
            r"UNLOCK_WORLD\s*=\s*(\d+)", (root / "game/include" / header).read_text()
        ).group(1))
    defined = {}
    for path in sorted((root / "game/include/level").glob("levels_world*.h")):
        for name in re.findall(r"(?:const|constexpr) LevelData (\w+)", path.read_text()):
            defined[name] = path
    bosses = set()
    for path in sorted((root / "game/levels").glob("*.json")):
        for lv in json.loads(path.read_text())["levels"]:
            if any(t.get("type") == "BOSS" for t in (lv.get("traps") or {}).get("items", [])):
                bosses.add(lv["name"])
    return [
        CampaignLevel(i, name, defined[name],
                      dash_allowed(i, worlds, unlock("cfg_dash.h")),
                      dash_allowed(i, worlds, unlock("cfg_wall_jump.h")),
                      dash_allowed(i, worlds, unlock("cfg_pogo.h")) or name in bosses)
        for i, name in enumerate(order)
    ]


def outside_play_area(spawn, door, world_width):
    """Which of "spawn" / "door" lies past the world-edge bounce
    (Core::set_horizontal_bound), where the duck cannot stay. The door
    counts as reached once the duck's body touches its 2 px point."""
    bound = world_width / 2 - EDGE_MARGIN
    reach = {"spawn": bound, "door": bound + BW / 2 + 1}
    return [name for name, (x, _) in (("spawn", spawn), ("door", door)) if abs(x) > reach[name]]


def load_world_widths(path):
    """{level: LevelData::world_width} (the screen width when not given)."""
    content = _ctr.strip_comments(Path(path).read_text())
    widths = {}
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        if content[m.end()] != "{":
            continue
        f = _ctr.top_level_fields(content[m.end() + 1 : _ctr.find_balanced_block(content, m.end())])
        widths[m.group(1)] = _num(f[10]) if len(f) > 10 else 240
    return widths


def outside_play_areas(levels):
    """'LEVEL: spawn' / 'LEVEL: door' for each campaign level whose spawn or
    door lies past its world-edge bounce."""
    loaded, widths, bad = {}, {}, []
    for level in levels:
        if level.path not in loaded:
            loaded[level.path] = load_levels(level.path)
            widths[level.path] = load_world_widths(level.path)
        _, _, spawn, door = loaded[level.path][level.name]
        bad += [f"{level.name}: {what}" for what in
                outside_play_area(spawn, door, widths[level.path][level.name])]
    return bad


def unreachable_doors(levels):
    """Names of the campaign levels whose door the duck cannot reach from a
    conservative start with the abilities of that level."""
    loaded = {}
    traps = {}
    bad = []
    for level in levels:
        if level.path not in loaded:
            loaded[level.path] = load_levels(level.path)
            traps[level.path] = load_static_traps(level.path)
        platforms, triggers, spawn, door = loaded[level.path][level.name]
        attach_lever_platforms(platforms, triggers)
        _, ok, _ = reach_with_levers(platforms, triggers, spawn, door, level.dash, False, level.wall)
        # the pogo only adds reach: simulated only where it is needed
        if not ok and level.pogo:
            _, ok, _ = reach_with_levers(
                platforms, triggers, spawn, door, level.dash, False, level.wall,
                traps[level.path].get(level.name, []))
        if not ok:
            bad.append(level.name)
    return bad


def main_all(root="."):
    levels = campaign(root)
    bad = unreachable_doors(levels)
    for level in levels:
        state = "NO DOOR" if level.name in bad else "ok"
        abilities = "+".join(
            n for n, on in (("dash", level.dash), ("pogo", level.pogo), ("wall", level.wall)) if on) or "-"
        print(f"{level.index:2d} {level.name:24s} {abilities:15s} {state}")
    print(f"\nReachability check: {len(bad)} of {len(levels)} level(s) cannot be finished.")
    outside = outside_play_areas(levels)
    for entry in outside:
        print(f"OUTSIDE THE PLAY AREA {entry}")
    return 1 if bad or outside else 0


def main(argv):
    args = argv[:]
    if args == ["--all"]:
        return main_all()
    if len(args) < 2:
        print(__doc__)
        return 2
    path, names = args[0], args[1:]
    levels = load_levels(path)
    bad = False
    for name in names:
        platforms, triggers, spawn, door = levels[name]
        print(f"== {name}: {len(platforms)} platforms, {len(triggers)} triggers")
        for dash, wall in ((False, False), (True, False), (True, True)):
            res = check_level(platforms, triggers, spawn, door, dash, wall)
            for key, (r, ok, n) in res.items():
                print(
                    f"  {key[0]:12s} {key[1]:8s} {'wall' if wall else '':4s} reached "
                    f"{len(r):2d}/{len(platforms)} door={'yes' if ok else 'NO '} configs={n}"
                )
        cons_dash = check_level(platforms, triggers, spawn, door, True, True)[("conservative", "dash")]
        if not cons_dash[1]:
            bad = True
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
