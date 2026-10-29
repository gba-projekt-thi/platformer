#!/usr/bin/env python3
"""
Check_level_reachability.py

Static jump/dash reachability check for the level data in
game/include/level/levels_world*.h. The duck's per-frame movement is the
game's own: tools/duck_sim/duck_sim.cpp builds locomotion::Core and
PhysicsBody's move for the host (`make duck-sim`, run by this script), so a
change to the movement rules or to cfg.h needs no change here.

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

Level Devil death windows (PlatformData / TrapData::from_death, until_death):
--all checks the level once per distinct set of objects that are there
(death_breakpoints()), so the door has to be reachable on every attempt,
whatever has moved by then - from the spawn and from a respawn at every
checkpoint. A respawn starts with the levers reset, except those that were
on when the flag was touched (CheckpointRun::restore()); assumed: any lever
configuration in which the duck reaches the flag's platform, on any attempt
(respawn_configs()). Every spot of a door that moves between attempts
(DoorData::attempt_positions) is checked.

False exits (DoorData::trap_trigger_name): when that trigger is a Latch or a
Toggle (it stays on once touched), --all also wants a route to the door that
never touches the trigger's zone (a jump touching it ends there); a Latch
that is on from the start is always reported. Hold / Timer triggers let go
by themselves and are not checked.

Traps are ignored (this checks geometry, not hazard timing). Wall jumps
(from Cfg::WallJump::UNLOCK_WORLD on) are simulated frame by frame by duck_sim,
with a few climb policies (between two walls, up one wall).

Usage (from the repository root):
    python3 tools/Check_level_reachability.py --all
        Every level of main.cpp's levels[] in play order; the dash and wall
        jumps count only from their Cfg::...::UNLOCK_WORLD on, as in the game.
    python3 tools/Check_level_reachability.py --all --mirror
        The same for the mirrored levels of Mirror Mode (mirror_world*.h).
    python3 tools/Check_level_reachability.py FILE LEVEL...
        Detailed report (with and without dash, both start assumptions) for
        the named levels of one file, for every spot of their door.

Exit code 0 if every checked level's door is reachable conservatively (with
the dash where --all allows it, always with it for FILE LEVEL...); 1
otherwise. The module is also imported by tools/test_check_level_reachability.py.
"""

import importlib.util
import json
import re
import subprocess
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
_spec = importlib.util.spec_from_file_location(
    "generate_levels", _here / "Generate_levels.py"
)
_gen = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_gen)
_STRUCTS = _gen.read_structs()


def field_index(struct, field):
    """Position of a PlatformData / TrapData field in the generated
    (positional) literals, read from level_structure.h."""
    return [name for name, _ in _STRUCTS[struct]].index(field)


def _window(fields, struct):
    """(from_death, until_death) of a literal; 0, 0 when they are left out."""
    out = []
    for name in ("from_death", "until_death"):
        i = field_index(struct, name)
        out.append(int(_num(fields[i])) if len(fields) > i else 0)
    return tuple(out)

# ---- the duck's movement comes from the game ------------------------------
# tools/duck_sim/duck_sim.cpp is locomotion::Core built for the host
# (`make duck-sim`); this module only plans and asks it to play each jump.
_REPO = Path(__file__).resolve().parent.parent
DUCK_SIM = _REPO / "build/host-tools/duck_sim"


class _DuckSim:
    """The duck_sim process (line protocol in its header comment)."""

    def __init__(self):
        subprocess.run(["make", "-s", "-C", str(_REPO), "duck-sim"], check=True)
        self.proc = subprocess.Popen(
            [str(DUCK_SIM)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        self.scene_key = None

    def ask(self, lines, answers=None):
        """Sends the lines, returns the answers (one per line unless said)."""
        self.proc.stdin.write("\n".join(lines) + "\n")
        self.proc.stdin.flush()
        return [self.proc.stdout.readline().split()
                for _ in range(len(lines) if answers is None else answers)]

    def use_scene(self, plats, door, targets, forbid):
        """Hands the platforms of a search to the process (once per scene)."""
        key = (tuple(plats), door and tuple(door), tuple(targets), tuple(forbid))
        if key == self.scene_key:
            return
        def boxes(rects):
            return " ".join(
                [str(len(rects))] + [f"{r.cx!r} {r.cy!r} {r.w!r} {r.h!r}" for r in rects])
        d = "0" if door is None else "1 " + " ".join(repr(v) for v in (*door[:2], *door_size(door)))
        self.ask([f"scene {d} {boxes(plats)} {boxes(targets)} {boxes(forbid)}"], answers=0)
        self.scene_key = key


_SIM = None


def _sim():
    global _SIM
    if _SIM is None:
        _SIM = _DuckSim()
    return _SIM


def _outcome(answer, plats, trace):
    """(landed platform or None, door touched, trace points or None)."""
    landed = int(answer[0])
    pts = None
    if trace:
        n = int(answer[2])
        pts = [(float(answer[3 + 2 * i]), float(answer[4 + 2 * i])) for i in range(n)]
    return (plats[landed] if landed >= 0 else None), answer[1] == "1", pts


# Cfg::Player::BODY_WIDTH / BODY_HEIGHT / MAX_SPEED, Cfg::Screen::EDGE_MARGIN,
# Cfg::Player::DEATH_HEIGHT, as the game was built.
_info = _sim().ask(["info"])[0]
BW, BH, MAXS = float(_info[0]), float(_info[1]), float(_info[2])
EDGE_MARGIN = int(_info[3])  # the duck bounces back past world_width / 2 - this
# A door is a box (x, y, w, h) around its hitbox centre: door_logic.h's
# DOOR_BOX_<sprite height> (the opaque pixels of the picture, 2 px in). A bare
# (x, y) means the standard 32 px door.
_REPO = Path(__file__).resolve().parent.parent


def _door_boxes():
    """{sprite height: (w, h, offset_x, offset_y)} from door_logic.h."""
    text = (_REPO / "game/include/door_logic.h").read_text()
    return {
        int(size): tuple(float(v) for v in dims)
        for size, *dims in re.findall(
            r"DOOR_BOX_(\d+)\s*=\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}", text)
    }


DOOR_BOXES = _door_boxes()
DOOR_W, DOOR_H = DOOR_BOXES[32][:2]


def door_size(door):
    return door[2:4] if len(door) > 2 else (DOOR_W, DOOR_H)


def _sprite_height(name):
    """Height of the door sprite `bn::sprite_items::<name>` (32 if unknown)."""
    for path in (_REPO / "game/graphics").rglob(name + ".json"):
        return json.loads(path.read_text()).get("height", 32)
    return 32
class Rect:
    __slots__ = (
        "cx", "cy", "w", "h", "index", "trigger", "active_when_on",
        "crumble", "respawn", "from_death", "until_death",
    )

    def __init__(
        self, cx, cy, w, h, index=-1, trigger=None, active_when_on=True,
        crumble=0, respawn=0, from_death=0, until_death=0,
    ):
        # PlatformData / TrapData::from_death, until_death (Level Devil): only
        # there while the duck has died [from, until) times, see present().
        self.from_death = from_death
        self.until_death = until_death
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


# Policy shapes (the movement itself is duck_sim's).
NEVER = 10**6  # `release`: the direction is held for the whole jump
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
            for release in (NEVER, 18, 12):
                if not jump and release != NEVER:
                    continue
                for dash_t in dash_ts:
                    yield d, jump, hold, dash_t, release


START_OFFSETS = (0.0, 10.0, 20.0, 30.0)  # px back from the edge (conservative)


def landings(active, start, door, allow_dash, optimistic, traces=None, allow_wall=False,
             pogo_targets=(), forbid=()):
    """Platforms (and door contact) reachable in ONE action from `start`,
    without touching a `forbid` box.

    `traces`, when a dict, collects one jump trajectory per
    (start index, landed index, dash scheduled) - the shortest one found."""
    # only traps a jump from here can reach (about 60 px past either edge)
    near = [t for t in pogo_targets if start.left - 80 < t.cx < start.right + 80]
    sim = _sim()
    sim.use_scene(active, door, near, forbid)
    at = active.index(start)
    trace = int(traces is not None)
    # every request of this start platform, answered in one go
    asks = []  # (request line, dash scheduled)
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
            asks.append((f"run {at} {x0!r} {vx0!r} {d} {int(jump)} {hold} "
                         f"{-1 if dash_t is None else dash_t} {release} {trace}", dash_t is not None))
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
                    asks.append((f"pogo {at} {x0!r} {vx0!r} {d} {r_from} {int(allow_wall)} 0", False))
    if allow_wall:
        for d, mode, slide, jump in WALL_POLICIES:
            edge = start.right if d == 1 else start.left
            for k in START_OFFSETS:
                x0 = edge - d * k
                if not start.left <= x0 <= start.right:
                    continue
                asks.append((f"climb {at} {x0!r} 0.0 {d} {int(mode == 'chimney')} {slide} "
                             f"{int(jump)} 0", False))
    found = set()
    door_hit = False
    for (line, dashed), answer in zip(asks, sim.ask([a for a, _ in asks])):
        landed, touched, points = _outcome(answer, active, bool(trace))
        door_hit |= touched
        if landed is not None and landed is not start:
            found.add(landed)
            if traces is not None and line.startswith("run"):
                key = (start.index, landed.index, dashed)
                if key not in traces or len(points) < len(traces[key]):
                    traces[key] = points
    return found, door_hit


def spawn_platform(active, spawn):
    """The platform a duck dropped at `spawn` lands on (None: it falls out)."""
    _sim().use_scene(active, None, (), ())
    landed = int(_sim().ask([f"spawn {spawn[0]!r} {spawn[1]!r}"])[0][0])
    return active[landed] if landed >= 0 else None


_REACH_CACHE = {}


def reach(active, spawn, door, allow_dash, optimistic, allow_wall=False, pogo_targets=(),
          forbid=(), until_door=False):
    """Returns (reached platforms, door reachable) for a fixed platform set,
    never touching a `forbid` box (see unreachable_windows()). Remembered
    per argument set: the checkpoint check asks again what the door check
    already simulated. `until_door` stops as soon as the door is touched (the
    reached set is then incomplete)."""
    key = (tuple(active), tuple(spawn), door and tuple(door), allow_dash, optimistic, allow_wall,
           tuple(pogo_targets), tuple(forbid), until_door)
    if key not in _REACH_CACHE:
        _REACH_CACHE[key] = _reach(active, spawn, door, allow_dash, optimistic, allow_wall,
                                   pogo_targets, forbid, until_door)
    reached, ok = _REACH_CACHE[key]
    return set(reached), ok


def _reach(active, spawn, door, allow_dash, optimistic, allow_wall, pogo_targets, forbid, until_door):
    first = spawn_platform(active, spawn)
    if first is None:
        return set(), False
    if forbid and (_hits(forbid, spawn[0], spawn[1], BW, BH) is not None
                   or _hits(forbid, spawn[0], first.top - BH / 2, BW, BH) is not None):
        return set(), False
    reached = {first}
    frontier = [first]
    door_ok = False
    while frontier and not (until_door and door_ok):
        cur = frontier.pop()
        found, touched = landings(active, cur, door, allow_dash, optimistic, allow_wall=allow_wall,
                                  pogo_targets=pogo_targets, forbid=forbid)
        door_ok |= touched
        for p in found:
            if p not in reached:
                reached.add(p)
                frontier.append(p)
    # standing on a reached platform next to the door also counts (unless the
    # walk there could cross a forbid box on that platform)
    if door is not None:
        for p in reached:
            if forbid and _hits(forbid, p.cx, p.top - BH / 2, p.w, BH) is not None:
                continue
            if overlap(
                _clamp(door[0], p.left, p.right),
                p.top - BH / 2,
                BW,
                BH,
                door[0],
                door[1],
                *door_size(door),
            ):
                door_ok = True
    return reached, door_ok


def present(obj, deaths):
    """attempt_variant::active_for_deaths(): is a platform / trap with a
    death window there after `deaths` deaths in the level (until 0 = no end)?"""
    return deaths >= obj.from_death and (obj.until_death == 0 or deaths < obj.until_death)


def death_breakpoints(objects):
    """The death counts at which the set of present objects can change, from
    0 on: checking each of them covers every attempt of the level."""
    points = {0}
    for obj in objects:
        points.update(d for d in (obj.from_death, obj.until_death) if d)
    return sorted(points)


def layouts(platforms, pogo_targets=()):
    """[(deaths, platforms there, pogo traps there)] for every
    death_breakpoints() layout of the level."""
    return [
        (deaths, [p for p in platforms if present(p, deaths)],
         [t for t in pogo_targets if present(t, deaths)])
        for deaths in death_breakpoints(list(platforms) + list(pogo_targets))
    ]


def _door_ok(active, targets, triggers, spawn, door, allow_dash, optimistic, allow_wall,
             start=None, forbid=(), until_door=False):
    def ok_with(pogo):
        return any(ok for _, _, ok in _explore_levers(
            active, triggers, spawn, door, allow_dash, optimistic, allow_wall, pogo, start, forbid,
            until_door))
    # the pogo only adds reach: simulated only where it is needed
    return ok_with(()) or (bool(targets) and ok_with(targets))


def unreachable_windows(platforms, triggers, spawn, door, allow_dash, optimistic,
                        allow_wall=False, pogo_targets=(), forbid=()):
    """The death counts (death_breakpoints()) after which the door cannot be
    reached with the platforms and pogo traps that are there then. [] = the
    level can be finished on every attempt. With `forbid` (the zone of a
    false exit's Latch trap trigger) the routes must not touch those boxes."""
    attach_lever_platforms(platforms, triggers)
    return [
        deaths for deaths, active, targets in layouts(platforms, pogo_targets)
        if not _door_ok(active, targets, triggers, spawn, door, allow_dash, optimistic, allow_wall,
                        forbid=forbid)
    ]


def flag_support(platforms, checkpoint):
    """The platform a checkpoint flag (16x16, centred on the respawn point)
    stands on, as Check_checkpoints.py finds it; None for a floating flag."""
    x, y = checkpoint
    for p in platforms:
        if p.left <= x <= p.right and 0 <= p.top - (y + 8) <= 4:
            return p
    return None


def respawn_configs(platforms, triggers, spawn, checkpoint, allow_dash, optimistic,
                    allow_wall=False, pogo_targets=(), door=None):
    """The lever configurations a respawn at `checkpoint` can start with.

    The game resets every trigger on a death and then switches on again the
    ones that were on when the flag was touched (CheckpointRun::restore()).
    Assumed conservatively: the flag can be touched in every configuration
    in which the duck reaches its platform from the spawn, on any attempt
    (a flag touched early still counts after later deaths). A floating flag
    counts as touched everywhere. {} = it is never touched. `door` only
    lets reach() reuse what the door check simulated."""
    attach_lever_platforms(platforms, triggers)
    names = sorted(triggers)
    if not names:
        # no lever: only one configuration, whether the flag is touched or not
        return {()}
    out = set()
    for _, active, targets in layouts(platforms, pogo_targets):
        support = flag_support(active, checkpoint)
        for cfg, reached, _ in _explore_levers(active, triggers, spawn, door, allow_dash, optimistic,
                                               allow_wall, targets):
            if support is None or support in reached:
                out.add(tuple((n, _default_on(triggers, n) or cfg[n]) for n in names))
    return out


def checkpoint_unreachable_windows(platforms, triggers, spawn, checkpoint, door, allow_dash,
                                   optimistic, allow_wall=False, pogo_targets=()):
    """Like unreachable_windows(), from a respawn at `checkpoint`: the death
    counts after which the duck respawning there with any of the lever
    configurations of respawn_configs() cannot reach the door."""
    configs = respawn_configs(platforms, triggers, spawn, checkpoint, allow_dash, optimistic,
                              allow_wall, pogo_targets, door)
    bad = []
    for deaths, active, targets in layouts(platforms, pogo_targets):
        for cfg in sorted(configs):
            if not _door_ok(active, targets, triggers, checkpoint, door, allow_dash, optimistic,
                            allow_wall, start=dict(cfg), until_door=True):
                bad.append(deaths)
                break
    return bad


def _clamp(v, lo, hi):
    return max(lo, min(hi, v))


def _default_on(triggers, name):
    # A boss's "<arena>_won" trigger (BossTrap) counts as on: the check
    # assumes the fight is won (the emulator replays prove that).
    return triggers[name]["default_on"] or name.endswith("_won")


def reach_with_levers(platforms, triggers, spawn, door, allow_dash, optimistic, allow_wall=False,
                      pogo_targets=(), start=None, forbid=()):
    """Fixpoint over lever configurations. `triggers` maps name -> dict with
    default_on and the Rect of the platform the lever stands on (or None);
    `start` ({name: on}) replaces the level's initial configuration.
    Returns (reached_ids, door_ok, configs_explored)."""
    all_reached = set()
    door_ok = False
    explored = 0
    for _, reached, ok in _explore_levers(platforms, triggers, spawn, door, allow_dash, optimistic,
                                          allow_wall, pogo_targets, start, forbid):
        door_ok |= ok
        all_reached |= {p.index for p in reached}
        explored += 1
    return all_reached, door_ok, explored


def _explore_levers(platforms, triggers, spawn, door, allow_dash, optimistic, allow_wall=False,
                    pogo_targets=(), start=None, forbid=(), until_door=False):
    """Yields (configuration, reached platforms, door reached) for every lever
    configuration the duck can set, from `start` or the level's initial one
    (`until_door`: see reach())."""
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

    first = tuple((n, start[n] if start is not None else _default_on(triggers, n)) for n in names)
    seen = {first}
    todo = [first]
    while todo:
        cfg = dict(todo.pop())
        active = active_for(cfg)
        reached, ok = reach(active, spawn, door, allow_dash, optimistic, allow_wall, pogo_targets,
                            forbid, until_door)
        yield cfg, reached, ok
        for n in names:
            holder = triggers[n]["platform"]
            if holder is not None and holder in reached:
                flipped = dict(cfg)
                flipped[n] = not cfg[n]
                key = tuple((k, flipped[k]) for k in names)
                if key not in seen:
                    seen.add(key)
                    todo.append(key)


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
    flee_paths = _point_arrays(content)
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
                start, end = _window(pf, "PlatformData")
                platforms.append(Rect(x + ox, y + oy, w, h, i, trig, awo, crumble, respawn,
                                      start, end))
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
        spawn = tuple(_num(v) for v in _fields_of(f[8])[:2])
        # The door of the first attempt (see load_door_positions()).
        door_pos = _door_positions(_fields_of(f[9]), flee_paths)[0]
        levels[name] = (platforms, triggers, spawn, door_pos)
    return levels


def _point_arrays(content):
    """{name: [(x, y), ...]} of every bn::fixed_point array (flee paths, a
    door's attempt positions)."""
    return {
        m.group(1): [(_num(x), _num(y)) for x, y in re.findall(r"\{\s*([-\d.]+)\s*,\s*([-\d.]+)\s*\}", m.group(2))]
        for m in re.finditer(r"constexpr bn::fixed_point (\w+)\[\]\s*=\s*\{(.*?)\};", content, re.S)
    }


def _door_positions(door, points):
    """Where the duck has to touch the door: its hitbox (x, y, w, h), for each
    spot it can stand on. DoorData::attempt_positions moves its origin after
    deaths, and a running door (flee_path) has to be reached where it ends up,
    its last stop."""
    match = re.search(r"sprite_items::(\w+)", door[2]) if len(door) > 2 else None
    w, h, ox, oy = DOOR_BOXES.get(_sprite_height(match.group(1)) if match else 32, DOOR_BOXES[32])
    x, y = _num(door[0]) + ox, _num(door[1]) + oy
    flee = points.get(door[6].strip(), [(0.0, 0.0)])[-1] if len(door) > 6 else (0.0, 0.0)
    spots = points.get(door[10].strip(), [(0.0, 0.0)]) if len(door) > 10 else [(0.0, 0.0)]
    return [(x + sx + flee[0], y + sy + flee[1], w, h) for sx, sy in spots]


def load_door_positions(path):
    """{level: [door position, ...]} - every spot the level's door can stand
    on (one for an ordinary door), the first attempt's first."""
    content = _ctr.strip_comments(Path(path).read_text())
    points = _point_arrays(content)
    out = {}
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        ob = m.end()
        if content[ob] != "{":
            continue
        f = _ctr.top_level_fields(content[ob + 1 : _ctr.find_balanced_block(content, ob)])
        out[m.group(1)] = _door_positions(_fields_of(f[9]), points)
    return out


def _level_fields(content):
    """(name, top-level fields) of every LevelData literal in `content`."""
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        ob = m.end()
        if content[ob] != "{":
            continue
        yield m.group(1), _ctr.top_level_fields(content[ob + 1: _ctr.find_balanced_block(content, ob)])


def load_checkpoints(path):
    """{level: [(x, y), ...]} - the respawn points of its checkpoints
    (LevelData::checkpoints), in play order."""
    content = _ctr.strip_comments(Path(path).read_text())
    arrays = _ctr.extract_named_arrays(content, "CheckpointData")
    field = field_index("LevelData", "checkpoints")
    out = {}
    for name, f in _level_fields(content):
        points = []
        if len(f) > field and f[field].strip() in arrays:
            for lit in _ctr.extract_top_level_literals(arrays[f[field].strip()]):
                cf = _ctr.top_level_fields(lit)
                points.append((_num(cf[0]), _num(cf[1])))
        out[name] = points
    return out


def load_door_traps(path):
    """{level: trigger or None} - the trigger that turns the level's door into
    a trap (DoorData::trap_trigger_name), as {name, mode, default_on, rect
    (cx, cy, w, h)}; None for an ordinary door or an unknown name."""
    content = _ctr.strip_comments(Path(path).read_text())
    arrays = _ctr.extract_named_arrays(content, "TriggerData")
    door_field = field_index("DoorData", "trap_trigger_name")
    mode_field = field_index("TriggerData", "mode")
    out = {}
    for name, f in _level_fields(content):
        door = _fields_of(f[field_index("LevelData", "door")])
        trap = None
        wanted = door[door_field].strip() if len(door) > door_field else "nullptr"
        array = f[field_index("LevelData", "triggers")].strip()
        if wanted.startswith('"') and array in arrays:
            for lit in _ctr.extract_top_level_literals(arrays[array]):
                tf = _ctr.top_level_fields(lit)
                if len(tf) > 5 and tf[5].strip() == wanted:
                    mode = tf[mode_field].split("::")[-1].strip() if len(tf) > mode_field else "Latch"
                    trap = {
                        "name": wanted.strip('"'),
                        "mode": mode,
                        "default_on": len(tf) > 4 and tf[4].strip() == "true",
                        "rect": tuple(_num(v) for v in tf[:4]),
                    }
        out[name] = trap
    return out


# Trigger modes that stay on once the duck has touched the zone (a Toggle
# needs a second visit to switch off, which the check does not try).
STAYS_ON = ("Latch", "Toggle")


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
                    start, end = _window(tf, "TrapData")
                    traps.append(Rect(x + ox, y + oy, w, h, i, from_death=start, until_death=end))
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


def campaign(root, mirror=False):
    """Every level of the game in play order (main.cpp's levels[]), with the
    file that defines it and whether the dash, wall jumps and the pogo are
    available there (the pogo also in every level with a boss). With `mirror`
    the mirrored levels (Mirror Mode, mirror_world*.h, NAME_MIRROR) instead."""
    root = Path(root)
    order = _cwi.parse_levels((root / "game/src/main.cpp").read_text())
    worlds = _cwi.parse_worlds((root / "game/include/world_index.h").read_text())
    def unlock(header):
        return int(re.search(
            r"UNLOCK_WORLD\s*=\s*(\d+)", (root / "game/include" / header).read_text()
        ).group(1))
    defined = {}
    pattern = "mirror_world*.h" if mirror else "levels_world*.h"
    for path in sorted((root / "game/include/level").glob(pattern)):
        for name in re.findall(r"(?:const|constexpr) LevelData (\w+)", path.read_text()):
            defined[name] = path
    bosses = set()
    for path in sorted((root / "game/levels").glob("*.json")):
        for lv in json.loads(path.read_text())["levels"]:
            if any(t.get("type") == "BOSS" for t in (lv.get("traps") or {}).get("items", [])):
                bosses.add(lv["name"])
    suffix = "_MIRROR" if mirror else ""
    return [
        CampaignLevel(i, name + suffix, defined[name + suffix],
                      dash_allowed(i, worlds, unlock("cfg_dash.h")),
                      dash_allowed(i, worlds, unlock("cfg_wall_jump.h")),
                      dash_allowed(i, worlds, unlock("cfg_pogo.h")) or name in bosses)
        for i, name in enumerate(order)
    ]


def outside_play_area(spawn, door, world_width):
    """Which of "spawn" / "door" lies past the world-edge bounce
    (Core::set_horizontal_bound), where the duck cannot stay. The door
    counts as reached once the duck's body touches its box."""
    bound = world_width / 2 - EDGE_MARGIN
    reach = {"spawn": bound, "door": bound + BW / 2 + door_size(door)[0] / 2}
    return [name for name, pos in (("spawn", spawn), ("door", door)) if abs(pos[0]) > reach[name]]


def load_world_widths(path):
    """{level: LevelData::world_width} (the screen width when not given)."""
    content = _ctr.strip_comments(Path(path).read_text())
    widths = {}
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        if content[m.end()] != "{":
            continue
        f = _ctr.top_level_fields(content[m.end() + 1 : _ctr.find_balanced_block(content, m.end())])
        # Not written (or spelled Cfg::Screen::WIDTH): the screen width.
        widths[m.group(1)] = _num(f[10]) if len(f) > 10 and f[10].strip()[0].isdigit() else 240
    return widths


def outside_play_areas(levels):
    """'LEVEL: spawn' / 'LEVEL: door' for each campaign level whose spawn or
    door lies past its world-edge bounce."""
    loaded, widths, doors, bad = {}, {}, {}, []
    for level in levels:
        if level.path not in loaded:
            loaded[level.path] = load_levels(level.path)
            widths[level.path] = load_world_widths(level.path)
            doors[level.path] = load_door_positions(level.path)
        _, _, spawn, _ = loaded[level.path][level.name]
        found = []
        # every spot of a door that moves between attempts
        for door in doors[level.path][level.name]:
            found += [w for w in outside_play_area(spawn, door, widths[level.path][level.name])
                      if w not in found]
        bad += [f"{level.name}: {what}" for what in found]
    return bad


def unreachable_doors(levels):
    """Names of the campaign levels whose door the duck cannot reach from a
    conservative start with the abilities of that level, with what fails in
    brackets: "(after 3 deaths)", "(from checkpoint 1 after 3 deaths)" (a
    respawn at that flag), "(door only through its trap trigger 'x')" (a
    false exit whose Latch / Toggle trigger zone lies on every route)."""
    loaded = {}
    bad = []

    def report(level, what, deaths):
        after = f"after {deaths} deaths" if deaths else ""
        detail = " ".join(part for part in (what, after) if part)
        name = f"{level.name} ({detail})" if detail else level.name
        if name not in bad:
            bad.append(name)

    for level in levels:
        if level.path not in loaded:
            loaded[level.path] = (load_levels(level.path), load_static_traps(level.path),
                                  load_door_positions(level.path), load_checkpoints(level.path),
                                  load_door_traps(level.path))
        levels_, traps, doors, checkpoints, door_traps = loaded[level.path]
        platforms, triggers, spawn, _ = levels_[level.name]
        targets = traps.get(level.name, []) if level.pogo else ()
        trap = door_traps.get(level.name)
        # Level Devil: every attempt counts, whatever has moved by then. Every
        # spot of a door that moves between attempts is checked against every
        # death window - more pairs than the game can show, never fewer.
        for door in doors[level.name]:
            failed = unreachable_windows(platforms, triggers, spawn, door, level.dash, False,
                                         level.wall, targets)
            for deaths in failed:
                report(level, "", deaths)
            # A false exit whose trigger stays on: the door has to be
            # reachable without touching the trigger zone.
            if trap is not None and trap["mode"] in STAYS_ON:
                what = f"door only through its trap trigger '{trap['name']}'"
                if trap["default_on"] and trap["mode"] == "Latch":
                    report(level, what, 0)
                else:
                    zone = [Rect(*trap["rect"])]
                    for deaths in unreachable_windows(platforms, triggers, spawn, door, level.dash,
                                                      False, level.wall, targets, zone):
                        if deaths not in failed:
                            report(level, what, deaths)
            # A respawn at a checkpoint has to reach the door as well.
            for i, point in enumerate(checkpoints.get(level.name, []), 1):
                for deaths in checkpoint_unreachable_windows(
                        platforms, triggers, spawn, point, door, level.dash, False, level.wall,
                        targets):
                    report(level, f"from checkpoint {i}", deaths)
    return bad


def main_all(root=".", mirror=False):
    levels = campaign(root, mirror)
    bad = unreachable_doors(levels)
    for level in levels:
        mine = [b for b in bad if b == level.name or b.startswith(level.name + " (")]
        state = "NO DOOR " + ", ".join(b[len(level.name):].strip() or "(at first)" for b in mine) \
            if mine else "ok"
        abilities = "+".join(
            n for n, on in (("dash", level.dash), ("pogo", level.pogo), ("wall", level.wall)) if on) or "-"
        print(f"{level.index:2d} {level.name:24s} {abilities:15s} {state}")
    failed = {b.split(" (")[0] for b in bad}
    print(f"\nReachability check: {len(failed)} of {len(levels)} level(s) cannot be finished.")
    outside = outside_play_areas(levels)
    for entry in outside:
        print(f"OUTSIDE THE PLAY AREA {entry}")
    return 1 if bad or outside else 0


def main(argv):
    args = argv[:]
    if args == ["--all"]:
        return main_all()
    if args == ["--all", "--mirror"]:
        return main_all(mirror=True)
    if len(args) < 2:
        print(__doc__)
        return 2
    path, names = args[0], args[1:]
    levels = load_levels(path)
    doors = load_door_positions(path)
    bad = False
    for name in names:
        platforms, triggers, spawn, _ = levels[name]
        print(f"== {name}: {len(platforms)} platforms, {len(triggers)} triggers")
        # every spot of a door that moves between attempts, as --all does
        for door in doors[name]:
            if len(doors[name]) > 1:
                print(f"  door at ({door[0]:g},{door[1]:g})")
            for dash, wall in ((False, False), (True, False), (True, True)):
                res = check_level(platforms, triggers, spawn, door, dash, wall)
                for key, (r, ok, n) in res.items():
                    print(
                        f"  {key[0]:12s} {key[1]:8s} {'wall' if wall else '':4s} reached "
                        f"{len(r):2d}/{len(platforms)} door={'yes' if ok else 'NO '} configs={n}"
                    )
                if (dash, wall) == (True, True) and not res[("conservative", "dash")][1]:
                    bad = True
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
