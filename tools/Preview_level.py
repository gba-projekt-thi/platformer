#!/usr/bin/env python3
"""
Preview_level.py

Draws a level of game/include/level/levels_world*.h as an SVG picture, in the
game's own coordinates (x to the right, y DOWN - smaller y is higher up - and
the world centered at (0, 0)), so a designer sees the layout, the hazards and
the jump arcs without building the ROM:

  * platforms, coloured by reachability: green = reachable with a plain jump,
    orange = only with the dash, red = not reachable at all; switched platforms
    are dashed and labelled "+lever" (solid while it is on) / "-lever"
  * triggers (Toggle / Hold marked), linked by a dotted line to the traps they
    release
  * traps by type: BASE (static), MOVING (ghost boxes every 12 frames after its
    trigger fires, until it leaves the world), PATH (the route and its nodes),
    CHASE (start, follow distance, speed), AMBUSH (trigger band and lunge)
  * checkpoints (teal flags, in play order), spawn, door, the death line (y = 100), the world bounds and one line per
    240 px "screen"
  * jump arcs between platforms, from the same movement model as
    tools/Check_level_reachability.py: blue = plain jump, orange dashed = needs
    the dash (only drawn where no plain jump exists)

Usage (from the repository root):

    python3 tools/Preview_level.py [FILE.h ...] [LEVEL ...] [options]

  FILE.h      level data file(s); default: every game/include/level/levels_world*.h
  LEVEL       level name(s), e.g. LEVEL_WORLD5_DASH; default: every level
  --out-dir D write D/<LEVEL>.svg (default build/level_preview), plus an
              index.html when more than one level is drawn
  -o FILE     output file (only with exactly one level)
  --arcs      none | plain | all (default all = plain jumps + dash jumps)
  --optimistic  draw arcs of the optimistic start (already past the edge)
  --no-traps  hide traps and triggers
  --no-reach  skip the reachability colouring (much faster)
  --scale S   pixels per game unit (default 2)

Pure Python, no third-party packages; the SVG opens in any browser or editor.
Traps are drawn, not simulated - this checks the picture, not fairness.
"""

import argparse
import glob
import html
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


_ctr = _load("check_trigger_references", "Check_trigger_references.py")
sim = _load("check_level_reachability", "Check_level_reachability.py")

SCREEN_W = 240
DEATH_Y = 100
GHOST_EVERY = 12  # frames between the ghost boxes of a MOVING trap
TRACK_FRAMES = 120
TRACK_MARGIN = 24  # how far past the world a MOVING trap's track is drawn
# Field order of TrapData (level_structure.h).
TRAP_FIELDS = (
    "type x y width height offset_x offset_y sprite sprite_waits graphic_indexes "
    "velocity_x velocity_y max_vel path path_waits chase_follow_distance chase_speed "
    "trigger_name ambush_range ambush_speed ambush_duration"
).split()


# ---------------------------------------------------------------- parsing --
def _num(text, default=0.0):
    try:
        return float(str(text).strip())
    except ValueError:
        return default


def _string(text):
    text = str(text).strip()
    return text[1:-1] if len(text) >= 2 and text[0] == '"' and text[-1] == '"' else None


def load_paths(directory):
    """NAME -> [(x, y), ...] for every `constexpr bn::fixed_point NAME[]`."""
    paths = {}
    for file in sorted(Path(directory).glob("levels_*.h")):
        text = _ctr.strip_comments(file.read_text())
        for m in re.finditer(r"constexpr bn::fixed_point (\w+)\[\]\s*=\s*\{", text):
            end = _ctr.find_balanced_block(text, m.end() - 1)
            body = text[m.end() : end]
            points = re.findall(r"fixed_point\(\s*([-\d.]+)\s*,\s*([-\d.]+)\s*\)", body)
            paths[m.group(1)] = [(float(a), float(b)) for a, b in points]
    return paths


def parse_traps(array_body, paths):
    traps = []
    for literal in _ctr.extract_top_level_literals(array_body):
        raw = _ctr.top_level_fields(literal)
        fields = dict(zip(TRAP_FIELDS, raw))
        kind = fields["type"].split("::")[-1]
        trap = {"type": kind}
        for key in ("x", "y", "width", "height", "offset_x", "offset_y"):
            trap[key] = _num(fields.get(key, 0))
        for key in (
            "velocity_x velocity_y max_vel chase_follow_distance chase_speed "
            "ambush_range ambush_speed ambush_duration path_waits"
        ).split():
            trap[key] = _num(fields.get(key, 0))
        trap["trigger"] = _string(fields.get("trigger_name", "nullptr"))
        trap["path"] = paths.get(fields.get("path", "").strip(), [])
        traps.append(trap)
    return traps


def parse_triggers(array_body):
    triggers = []
    for literal in _ctr.extract_top_level_literals(array_body):
        f = _ctr.top_level_fields(literal)
        mode = "Latch"
        if len(f) > 6 and "TriggerMode::" in f[6]:
            mode = f[6].split("::")[-1].strip()
        triggers.append(
            {
                "x": _num(f[0]),
                "y": _num(f[1]),
                "width": _num(f[2]),
                "height": _num(f[3]),
                "default_on": len(f) > 4 and f[4].strip() == "true",
                "name": _string(f[5]) if len(f) > 5 else None,
                "mode": mode,
                # TriggerData::delay_frames (field 10): lag, or timer length
                "delay": int(_num(f[10])) if len(f) > 10 else 0,
                # TriggerData::action (field 11): Enter / Jump / Dash
                "action": f[11].split("::")[-1].strip() if len(f) > 11 else "Enter",
                # TriggerData::sprite (field 7): a visible switch uses a sprite slot.
                "has_sprite": len(f) > 7 and f[7].strip() not in ("", "nullptr"),
            }
        )
    return triggers


class Level:
    def __init__(self, name):
        self.name = name
        self.world_w = 240.0
        self.world_h = 160.0
        self.platforms = []
        self.triggers = []
        self.traps = []
        self.checkpoints = []
        self.sim_triggers = {}
        self.spawn = (0.0, 0.0)
        self.door = (0.0, 0.0)
        self.door_hit = (0.0, 0.0)
        self.hint = None


def parse_checkpoints(array_body):
    points = []
    for literal in _ctr.extract_top_level_literals(array_body):
        f = _ctr.top_level_fields(literal)
        points.append((_num(f[0]), _num(f[1])))
    return points


def load_level_file(path):
    """Returns {level name: Level}."""
    path = Path(path)
    content = _ctr.strip_comments(path.read_text())
    base = sim.load_levels(path)
    paths = load_paths(path.parent)
    trap_arrays = _ctr.extract_named_arrays(content, "TrapData")
    trigger_arrays = _ctr.extract_named_arrays(content, "TriggerData")
    checkpoint_arrays = _ctr.extract_named_arrays(content, "CheckpointData")
    levels = {}
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        name = m.group(1)
        ob = m.end()
        if content[ob] != "{" or name not in base:
            continue
        cb = _ctr.find_balanced_block(content, ob)
        f = _ctr.top_level_fields(content[ob + 1 : cb])
        platforms, sim_triggers, spawn, door_hit = base[name]
        level = Level(name)
        level.platforms = platforms
        level.sim_triggers = sim_triggers
        level.spawn = spawn
        level.door_hit = door_hit
        level.door = (door_hit[0], door_hit[1] - 6.0)
        if f[2] != "nullptr" and f[2] in trigger_arrays:
            level.triggers = parse_triggers(trigger_arrays[f[2]])
        if f[4] != "nullptr" and f[4] in trap_arrays:
            level.traps = parse_traps(trap_arrays[f[4]], paths)
        if len(f) > 11:
            level.world_w, level.world_h = _num(f[10], 240.0), _num(f[11], 160.0)
        if len(f) > 13:
            level.hint = _string(f[13])
        # LevelData field 14 = checkpoints array, 15 = count
        if len(f) > 14 and f[14] in checkpoint_arrays:
            level.checkpoints = parse_checkpoints(checkpoint_arrays[f[14]])
        levels[name] = level
    return levels


# ---------------------------------------------------------- geometry --
def trap_position(trap):
    """World position of the trap's collision box (BaseTrap adds the offset;
    a PathTrap starts on its first path node)."""
    px = py = 0.0
    if trap["type"] == "PATH" and trap["path"]:
        px, py = trap["path"][0]
    return trap["x"] + px + trap["offset_x"], trap["y"] + py + trap["offset_y"]


def moving_track(trap, frames=TRACK_FRAMES):
    """Positions of a MovingTrap after each frame its trigger is on: velocity
    gains the acceleration, is clamped by magnitude to max_vel (0 = never
    moves, like PhysicsBody), then moves. Collisions are ignored."""
    x, y = trap_position(trap)
    vx = vy = 0.0
    track = [(x, y)]
    for _ in range(frames):
        vx += trap["velocity_x"]
        vy += trap["velocity_y"]
        speed = math.hypot(vx, vy)
        limit = trap["max_vel"]
        if speed > limit:
            scale = limit / speed if speed else 0.0
            vx, vy = vx * scale, vy * scale
        x, y = x + vx, y + vy
        track.append((x, y))
    return track


def lever_configs(level):
    names = sorted(level.sim_triggers)
    return names, tuple(level.sim_triggers[n]["default_on"] for n in names)


def compute_arcs(level, allow_dash, optimistic):
    """{(start index, landed index, used dash): [(x, y), ...]} over every
    lever configuration the duck can set from platforms it has reached."""
    sim.attach_lever_platforms(level.platforms, level.sim_triggers)
    names, start_cfg = lever_configs(level)
    seen, todo, traces = {start_cfg}, [start_cfg], {}
    cache = {}  # (active platform indexes, start index) -> landed platforms
    while todo:
        cfg = todo.pop()
        state = dict(zip(names, cfg))
        active = [
            p
            for p in level.platforms
            if p.trigger is None or state[p.trigger] == p.active_when_on
        ]
        first = sim.spawn_platform(active, level.spawn)
        if first is None:
            continue
        reached, frontier = {first}, [first]
        while frontier:
            current = frontier.pop()
            key = (tuple(sorted(q.index for q in active)), current.index)
            if key not in cache:
                cache[key], _ = sim.landings(
                    active, current, level.door_hit, allow_dash, optimistic, traces
                )
            found = cache[key]
            for p in found:
                if p not in reached:
                    reached.add(p)
                    frontier.append(p)
        for i, name in enumerate(names):
            holder = level.sim_triggers[name]["platform"]
            if holder is not None and holder in reached:
                flipped = list(cfg)
                flipped[i] = not flipped[i]
                flipped = tuple(flipped)
                if flipped not in seen:
                    seen.add(flipped)
                    todo.append(flipped)
    return traces


def reachability(level, optimistic):
    sim.attach_lever_platforms(level.platforms, level.sim_triggers)
    plain, door_plain, _ = sim.reach_with_levers(
        level.platforms, level.sim_triggers, level.spawn, level.door_hit, False, optimistic
    )
    dash, door_dash, _ = sim.reach_with_levers(
        level.platforms, level.sim_triggers, level.spawn, level.door_hit, True, optimistic
    )
    return plain, dash, door_plain, door_dash


# ------------------------------------------------------------ drawing --
STYLE = """
  .bg { fill: #fbfbfd; }
  .world { fill: #ffffff; stroke: #8a8a99; stroke-width: 0.6; stroke-dasharray: 4 2; }
  .screenline { stroke: #d8d8e2; stroke-width: 0.4; stroke-dasharray: 2 2; }
  .death { stroke: #d32f2f; stroke-width: 0.5; stroke-dasharray: 3 2; }
  text { font-family: sans-serif; fill: #222; }
  .label { font-size: 5px; }
  .small { font-size: 4px; fill: #555; }
  .title { font-size: 8px; font-weight: bold; }
  .platform { fill: #9ccc87; stroke: #3d6b2e; stroke-width: 0.6; }
  .platform.dash-only { fill: #f6b67a; stroke: #b5651d; }
  .platform.unreachable { fill: #ef9a9a; stroke: #b71c1c; }
  .platform.switched { stroke-dasharray: 2 1; }
  .trigger { fill: #4aa3df; fill-opacity: 0.15; stroke: #1e78b4; stroke-width: 0.5; stroke-dasharray: 2 1; }
  .link { stroke: #1e78b4; stroke-width: 0.4; stroke-dasharray: 0.5 1.5; fill: none; }
  .trap { stroke-width: 0.6; fill-opacity: 0.35; }
  .trap-base { fill: #e53935; stroke: #b71c1c; }
  .trap-moving { fill: #fb8c00; stroke: #e65100; }
  .trap-path { fill: #8e24aa; stroke: #4a148c; }
  .trap-chase { fill: #d81b60; stroke: #880e4f; }
  .trap-ambush { fill: #795548; stroke: #3e2723; }
  .ghost { fill: none; stroke: #e65100; stroke-width: 0.4; stroke-dasharray: 1.5 1.5; }
  .track { fill: none; stroke: #e65100; stroke-width: 0.5; }
  .route { fill: none; stroke: #8e24aa; stroke-width: 0.5; stroke-dasharray: 3 1.5; }
  .node { fill: #8e24aa; }
  .band { fill: #795548; fill-opacity: 0.08; stroke: #795548; stroke-width: 0.3; stroke-dasharray: 1 1.5; }
  .lunge { fill: none; stroke: #795548; stroke-width: 0.7; }
  .chase-arrow { fill: none; stroke: #880e4f; stroke-width: 0.7; }
  .arc { fill: none; stroke-width: 0.6; stroke-linejoin: round; }
  .arc-plain { stroke: #1565c0; stroke-opacity: 0.55; }
  .arc-dash { stroke: #ef6c00; stroke-opacity: 0.8; stroke-dasharray: 2 1.2; }
  .spawn { fill: #2e7d32; stroke: #1b5e20; stroke-width: 0.5; }
  .door { fill: #ffd54f; fill-opacity: 0.45; stroke: #f9a825; stroke-width: 0.6; }
  .doorhit { fill: #5d4037; }
  .checkpoint { fill: #26a69a; fill-opacity: 0.35; stroke: #00695c; stroke-width: 0.7; }
"""


class Svg:
    def __init__(self):
        self.parts = []
        self.xs = []
        self.ys = []

    def extend(self, x0, y0, x1=None, y1=None):
        for x, y in ((x0, y0), (x1 if x1 is not None else x0, y1 if y1 is not None else y0)):
            self.xs.append(x)
            self.ys.append(y)

    def add(self, text):
        self.parts.append(text)

    def rect(self, cx, cy, w, h, cls, title=None):
        self.extend(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2)
        inner = f"<title>{html.escape(title)}</title>" if title else ""
        self.add(
            f'<rect class="{cls}" x="{cx - w / 2:g}" y="{cy - h / 2:g}" '
            f'width="{w:g}" height="{h:g}">{inner}</rect>'
        )

    def line(self, x0, y0, x1, y1, cls):
        self.extend(x0, y0, x1, y1)
        self.add(f'<line class="{cls}" x1="{x0:g}" y1="{y0:g}" x2="{x1:g}" y2="{y1:g}"/>')

    def polyline(self, points, cls, closed=False):
        if len(points) < 2:
            return
        for x, y in points:
            self.extend(x, y)
        data = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
        tag = "polygon" if closed else "polyline"
        self.add(f'<{tag} class="{cls}" points="{data}"/>')

    def text(self, x, y, text, cls="label", anchor="start"):
        self.extend(x, y - 5, x + 4 * len(text) * (1 if anchor == "start" else 0), y)
        self.add(
            f'<text class="{cls}" x="{x:g}" y="{y:g}" text-anchor="{anchor}">'
            f"{html.escape(text)}</text>"
        )


def _fmt(value):
    return f"{value:g}"


def render(level, arcs="all", optimistic=False, show_traps=True, scale=2.0, reach=True):
    """Returns the SVG text for `level`. reach=False skips the reachability
    colouring and the door line (much faster)."""
    svg = Svg()
    half_w, half_h = level.world_w / 2, level.world_h / 2
    svg.extend(-half_w, -half_h, half_w, half_h)
    svg.extend(-half_w, DEATH_Y)

    # world bounds + screens
    svg.add(
        f'<g id="world"><rect class="world" x="{-half_w:g}" y="{-half_h:g}" '
        f'width="{level.world_w:g}" height="{level.world_h:g}"/>'
    )
    screens = max(1, math.ceil(level.world_w / SCREEN_W - 1e-9))
    for i in range(1, screens):
        x = -half_w + i * SCREEN_W
        svg.add(
            f'<line class="screenline" x1="{x:g}" y1="{-half_h:g}" x2="{x:g}" y2="{half_h:g}"/>'
        )
    svg.add(
        f'<line class="death" x1="{-half_w:g}" y1="{DEATH_Y}" x2="{half_w:g}" y2="{DEATH_Y}"/>'
    )
    svg.add("</g>")
    svg.text(-half_w + 2, DEATH_Y - 2, f"death y={DEATH_Y}", "small")

    # reachability colours
    if reach:
        plain, dash, door_plain, door_dash = reachability(level, optimistic)
    else:
        plain = dash = {p.index for p in level.platforms}
        door_plain = door_dash = None

    # arcs first (under everything)
    arc_count = 0
    if arcs != "none":
        traces = compute_arcs(level, arcs == "all", optimistic)
        pairs_with_plain = {(a, b) for (a, b, d) in traces if not d}
        svg.add('<g id="arcs">')
        for (a, b, used_dash), points in sorted(traces.items()):
            if used_dash and (a, b) in pairs_with_plain:
                continue
            svg.polyline(points, "arc arc-dash" if used_dash else "arc arc-plain")
            arc_count += 1
        svg.add("</g>")

    # platforms
    svg.add('<g id="platforms">')
    for p in level.platforms:
        cls = "platform"
        if p.index not in plain:
            cls += " dash-only" if p.index in dash else " unreachable"
        title = f"platform {p.index} ({p.w:g}x{p.h:g}) at ({p.cx:g},{p.cy:g})"
        if p.trigger is not None:
            cls += " switched"
            title += f" - {'solid' if p.active_when_on else 'gone'} while {p.trigger} is on"
        svg.rect(p.cx, p.cy, p.w, p.h, cls, title)
        if p.trigger is not None:
            sign = "+" if p.active_when_on else "-"
            svg.text(p.cx, p.cy - p.h / 2 - 2, f"{sign}{p.trigger}", "small", "middle")
    svg.add("</g>")

    if show_traps:
        _draw_triggers_and_traps(svg, level)

    # spawn + door
    svg.add('<g id="markers">')
    svg.rect(level.spawn[0], level.spawn[1], 8, 8, "spawn", "player spawn")
    svg.text(level.spawn[0], level.spawn[1] - 7, "spawn", "small", "middle")
    svg.rect(level.door[0], level.door[1], 32, 32, "door", "door sprite")
    svg.rect(level.door_hit[0], level.door_hit[1], 3, 3, "doorhit", "door hit box")
    svg.text(level.door[0], level.door[1] - 18, "door", "small", "middle")
    svg.add("</g>")

    svg.add('<g id="checkpoints">')
    for i, (cx, cy) in enumerate(level.checkpoints):
        svg.rect(cx, cy, 16, 16, "checkpoint", f"checkpoint {i + 1} (respawn at {cx:g},{cy:g})")
        svg.text(cx, cy - 11, f"checkpoint {i + 1}", "small", "middle")
    svg.add("</g>")

    # header + legend
    counts = {}
    for t in level.traps:
        counts[t["type"]] = counts.get(t["type"], 0) + 1
    trap_text = " ".join(f"{k} {v}" for k, v in sorted(counts.items())) or "none"
    yes = lambda b: "yes" if b else "NO"
    door_line = (
        f"door reachable ({'optimistic' if optimistic else 'conservative'} start): "
        f"plain jump {yes(door_plain)}, with dash {yes(door_dash)}"
        if reach
        else "reachability not checked (--no-reach)"
    )
    lines = [
        f"{level.name}   world {_fmt(level.world_w)}x{_fmt(level.world_h)} "
        f"({screens} screen{'s' if screens != 1 else ''})",
        f"platforms {len(level.platforms)}   triggers {len(level.triggers)}   "
        f"checkpoints {len(level.checkpoints)}   "
        f"traps: {trap_text}   arcs drawn: {arc_count}",
        door_line,
    ]
    if level.hint:
        lines.append(f'hint: "{level.hint}"')
    top = min(svg.ys) - 12
    left = min(svg.xs) - 8
    header = [
        f'<text class="title" x="{left:g}" y="{top - 4 * len(lines) - 10:g}">'
        f"{html.escape(lines[0])}</text>"
    ]
    for i, line in enumerate(lines[1:], 1):
        header.append(
            f'<text class="label" x="{left:g}" y="{top - 4 * len(lines) - 10 + 7 * i:g}">'
            f"{html.escape(line)}</text>"
        )
    legend_items = [
        ("platform", "platform (plain jump)"),
        ("platform dash-only", "needs the dash"),
        ("platform unreachable", "unreachable"),
        ("trap trap-base", "BASE"),
        ("trap trap-moving", "MOVING"),
        ("trap trap-path", "PATH"),
        ("trap trap-chase", "CHASE"),
        ("trap trap-ambush", "AMBUSH"),
    ]
    legend_y = max(svg.ys) + 12
    legend = ['<g id="legend">']
    x = left
    for cls, label in legend_items:
        legend.append(f'<rect class="{cls}" x="{x:g}" y="{legend_y:g}" width="8" height="5"/>')
        legend.append(f'<text class="small" x="{x + 10:g}" y="{legend_y + 4.2:g}">{label}</text>')
        x += 14 + 3.2 * len(label)
    legend.append(
        f'<line class="arc arc-plain" x1="{x:g}" y1="{legend_y + 2.5:g}" '
        f'x2="{x + 8:g}" y2="{legend_y + 2.5:g}"/>'
        f'<text class="small" x="{x + 10:g}" y="{legend_y + 4.2:g}">plain jump arc</text>'
    )
    x += 14 + 3.2 * 14
    legend.append(
        f'<line class="arc arc-dash" x1="{x:g}" y1="{legend_y + 2.5:g}" '
        f'x2="{x + 8:g}" y2="{legend_y + 2.5:g}"/>'
        f'<text class="small" x="{x + 10:g}" y="{legend_y + 4.2:g}">dash arc</text>'
    )
    legend.append("</g>")

    min_x = left
    min_y = top - 4 * len(lines) - 18
    max_x = max(max(svg.xs) + 8, x + 40)
    max_y = legend_y + 10
    width, height = max_x - min_x, max_y - min_y
    body = "\n".join(svg.parts)
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'viewBox="{min_x:g} {min_y:g} {width:g} {height:g}" '
        f'width="{width * scale:g}" height="{height * scale:g}">\n'
        f"<style>{STYLE}</style>\n"
        f'<rect class="bg" x="{min_x:g}" y="{min_y:g}" width="{width:g}" height="{height:g}"/>\n'
        + "\n".join(header)
        + "\n"
        + body
        + "\n"
        + "\n".join(legend)
        + "\n</svg>\n"
    )


def trigger_note(t):
    """' [Toggle]', ' [Timer 120f]', ' +30f' ... - what sets a trigger apart
    from the plain latching zone."""
    delay = t.get("delay", 0)
    if t["mode"] == "Timer":
        return f" [Timer {delay}f]"
    note = "" if t["mode"] == "Latch" else f" [{t['mode']}]"
    action = t.get("action", "Enter")
    if action != "Enter":
        note += f" on {action.lower()}"
    return note + (f" +{delay}f" if delay else "")


def _draw_triggers_and_traps(svg, level):
    svg.add('<g id="triggers">')
    by_name = {}
    for t in level.triggers:
        mode = trigger_note(t)
        title = f"trigger {t['name'] or '(unnamed)'}{mode}"
        svg.rect(t["x"], t["y"], t["width"], t["height"], "trigger", title)
        if t["name"]:
            by_name[t["name"]] = t
            svg.text(
                t["x"], t["y"] + t["height"] / 2 + 5, f"{t['name']}{mode}", "small", "middle"
            )
    svg.add("</g>")

    svg.add('<g id="traps">')
    for trap in level.traps:
        kind = trap["type"]
        x, y = trap_position(trap)
        cls = f"trap trap-{kind.lower()}"
        title = f"{kind} trap at ({x:g},{y:g}) {trap['width']:g}x{trap['height']:g}"
        if trap["trigger"]:
            title += f" - trigger {trap['trigger']}"
            t = by_name.get(trap["trigger"])
            if t is not None:
                svg.line(t["x"], t["y"], x, y, "link")

        if kind == "MOVING":
            track = []
            for px, py in moving_track(trap):
                if abs(px) > level.world_w / 2 + TRACK_MARGIN or abs(py) > (
                    level.world_h / 2 + TRACK_MARGIN
                ):
                    break
                track.append((px, py))
            for gx, gy in track[GHOST_EVERY::GHOST_EVERY]:
                svg.rect(gx, gy, trap["width"], trap["height"], "ghost")
            svg.polyline(track, "track")
            svg.rect(x, y, trap["width"], trap["height"], cls, title)
            note = trap["trigger"] or "no trigger"
            if trap["max_vel"] == 0:
                note += " (max_vel 0: does not move)"
            svg.text(x, y - trap["height"] / 2 - 2, f"MOVING {note}", "small", "middle")
        elif kind == "PATH":
            route = [
                (trap["x"] + px + trap["offset_x"], trap["y"] + py + trap["offset_y"])
                for px, py in trap["path"]
            ]
            svg.polyline(route, "route", closed=True)
            for rx, ry in route:
                svg.add(f'<circle class="node" cx="{rx:g}" cy="{ry:g}" r="0.9"/>')
            svg.rect(x, y, trap["width"], trap["height"], cls, title)
            svg.text(
                x,
                y - trap["height"] / 2 - 2,
                f"PATH {trap['trigger'] or ''} {trap['path_waits']:g}f/node",
                "small",
                "middle",
            )
        elif kind == "AMBUSH":
            reach = trap["ambush_range"]
            half_h = level.world_h / 2
            svg.add(
                f'<rect class="band" x="{x - reach:g}" y="{-half_h:g}" '
                f'width="{2 * reach:g}" height="{level.world_h:g}"/>'
            )
            svg.extend(x - reach, -half_h, x + reach, half_h)
            travel = trap["ambush_speed"] * trap["ambush_duration"]
            svg.line(x, y, x + travel, y, "lunge")
            svg.rect(x + travel, y, trap["width"], trap["height"], "ghost")
            svg.rect(x, y, trap["width"], trap["height"], cls, title)
            svg.text(
                x,
                y - trap["height"] / 2 - 2,
                f"AMBUSH r={reach:g} lunge {travel:g}",
                "small",
                "middle",
            )
        elif kind == "CHASE":
            svg.rect(x, y, trap["width"], trap["height"], cls, title)
            svg.line(x, y, x + 24, y, "chase-arrow")
            svg.text(
                x,
                y - trap["height"] / 2 - 2,
                f"CHASE {trap['chase_follow_distance']:g}px behind, "
                f"{trap['chase_speed']:g}px/f",
                "small",
                "middle",
            )
        else:
            svg.rect(x, y, trap["width"], trap["height"], cls, title)
    svg.add("</g>")


# ---------------------------------------------------------------- CLI --
def index_html(names, scale_note=""):
    items = "\n".join(
        f'<h3>{html.escape(n)}</h3><img src="{html.escape(n)}.svg" alt="{html.escape(n)}">'
        for n in names
    )
    return (
        "<!doctype html><meta charset=utf-8><title>Level previews</title>"
        "<style>body{font-family:sans-serif;margin:16px}img{max-width:100%;"
        "border:1px solid #ccc}</style><h1>Level previews</h1>"
        f"{scale_note}\n{items}\n"
    )


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("items", nargs="*", help="level files (.h) and/or level names")
    parser.add_argument("--out-dir", default="build/level_preview")
    parser.add_argument("-o", dest="output")
    parser.add_argument("--arcs", choices=("none", "plain", "all"), default="all")
    parser.add_argument("--optimistic", action="store_true")
    parser.add_argument("--no-traps", action="store_true")
    parser.add_argument("--no-reach", action="store_true")
    parser.add_argument("--scale", type=float, default=2.0)
    args = parser.parse_args(argv)

    files = [Path(i) for i in args.items if i.endswith(".h")]
    wanted = [i for i in args.items if not i.endswith(".h")]
    if not files:
        files = sorted(Path(p) for p in glob.glob("**/levels_world*.h", recursive=True))
    if not files:
        print("No levels_world*.h files found.", file=sys.stderr)
        return 1

    levels = {}
    for file in files:
        levels.update(load_level_file(file))
    names = wanted or list(levels)
    missing = [n for n in names if n not in levels]
    if missing:
        print(f"Unknown level(s): {', '.join(missing)}", file=sys.stderr)
        print(f"Available: {', '.join(levels)}", file=sys.stderr)
        return 1
    if args.output and len(names) != 1:
        print("-o needs exactly one level; use --out-dir instead.", file=sys.stderr)
        return 1

    out_dir = Path(args.out_dir)
    for name in names:
        text = render(
            levels[name],
            args.arcs,
            args.optimistic,
            not args.no_traps,
            args.scale,
            not args.no_reach,
        )
        target = Path(args.output) if args.output else out_dir / f"{name}.svg"
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
        print(f"wrote {target}")
    if len(names) > 1 and not args.output:
        (out_dir / "index.html").write_text(index_html(names))
        print(f"wrote {out_dir / 'index.html'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
