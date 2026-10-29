#!/usr/bin/env python3
"""
Check_sprite_budget.py

Statically checks the sprite and entity budgets of the level data in
game/include/level/levels_world*.h - the numbers a level has to stay inside
long before anybody runs the ROM:

  * entity limits (Cfg::Level::Limits, read from cfg.h): platforms, triggers
    and traps per level, and ENTITY_SPRITES, the cap on the sprites of all of
    them together (the per-kind limits alone allow more sprites than the
    registry and the OAM hold). LevelManager::_validate_level() asserts the same at
    load time; this reports it at commit/CI time instead.
  * SpriteRegistry capacity (MAX_SPRITES, read from sprite_registry.h): every
    object that scrolls with the camera is a `Sprite` registered there - the
    duck, the door, platforms, traps, switch sprites, checkpoint flags and the
    dash afterimages. A sprite beyond the capacity is dropped by the registry
    (logged, but it then stays pinned to its screen position).
  * the 128 hardware sprite slots (OAM): the registered sprites plus the HUD
    (death counter, timer), the level hint, the
    "New Best!" banner and the pause overlay. Butano counts hidden sprites
    too, so all of them are allocated at once in the worst case - except the
    hint and the pause overlay: LevelScene releases the hint while paused, and
    PauseController creates its title and rows only while paused and releases
    them on resume.
  * registry capacities of the collision system (collision_registry.h) against
    the entity limits.

Level Devil objects with a death window (from_death / until_death) count on
every attempt: LevelManager creates all of them at load and only hides the
ones that are not there, so a trap that moves after the 3rd death (two copies)
costs two sprites and two traps.

Everything is a worst-case model, counted in drawn glyphs (spaces draw no
sprite). The text constants mirror pause_controller.cpp / level_scene.cpp /
death_counter.h - keep them in sync when those strings change (the tests
check the pause overlay against the capacity of its sprite vector).

Usage (from the repository root):
    python3 tools/Check_sprite_budget.py [file ...]

Errors (exit code 1): an entity limit, the registry capacity or the 128 slots
is exceeded by a level. Warnings: less than WARN_HEADROOM left, or the entity
limits together allow more sprites than the registry / OAM can hold (a
property of the configuration, not of a level).

Pure text parsing; reuses the level loader of Preview_level.py.
"""

import importlib.util
import json
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

OAM_SLOTS = 128  # GBA hardware: 128 sprites
WARN_HEADROOM = 8
PLAYER_AND_DOOR = 2  # the duck and the level's door are always registered
POGO_SLASH = 1  # Player's slash while a pogo strike reaches (cfg_pogo.h)

# Drawn glyphs of the always-present text (spaces draw nothing).
DEATH_COUNTER_GLYPHS = 12  # "Deaths:" (7) + up to 5 digits (uint16_t)
TIMER_GLYPHS = 8  # MM:SS:CC
PAUSE_TITLE_GLYPHS = 6  # "Paused", only while paused (not in the controls view)
HINT_MAX_GLYPHS = 28  # hint::MAX_CHARS
BANNER_LINES = ("New Best!", "01:23.45", "Press A to continue")

# pause_controller.cpp: main menu, options sub-menu and (controls_text.h)
# controls view. Only one view exists at a time, so the overlay costs the
# largest of them; each has one '>' cursor.
PAUSE_MAIN_ROWS = ("Continue", "Restart Level", "Options", "Controls", "Title Screen")
PAUSE_OPTIONS_ROWS = (
    "Music < 4/4 >",
    "SFX < 4/4 >",
    "Hard Mode < Off >",
    "Assist Mode < Off >",
    "Traps x1.35",  # info line, only while a mode is on
)
CONTROLS_TITLE = "Controls"
DEFAULT_CONTROLS_LINES = (
    "D-Pad Move", "A Jump/Wall W5+", "B Dash W3+", "R Pogo W4+", "Start Pause", "Select Timer",
)

# level_select_scene.cpp: a row per level of the longest world, "Lv 1*+", the
# selected one "> Lv 1 01:23.45*+", and above them the selected level's goals
# and (once the game is finished) the Mirror Mode line.
LEVEL_SELECT_ROW = "Lv 1*+"
LEVEL_SELECT_SELECTED_ROW = "> Lv 1 01:23.45*+"
LEVEL_SELECT_GOALS = "[*] No death  [+] <1:30"
LEVEL_SELECT_MIRROR = "Mirror off (Select)"

DEFAULTS = {
    "LEVEL_SELECT_ROWS": 8,
    "MAX_SPRITES": 72,
    "SLOTS": 4,  # DashTrail::SLOTS
    "PLATFORMS": 32,
    "PLATFORM_BODIES": 32,
    "TRIGGERS": 16,
    "TOTAL_TRAPS": 32,
    "CHECKPOINTS": 1,
    "ENTITY_SPRITES": 39,
    "MAX_STATIC_BODIES": 96,
    "MAX_PHYSICS_BODIES": 64,
    "MAX_COLLISION_SHAPES": 64,
}

_SKIP_DIRS = {"build", ".git", "node_modules"}


def glyphs(text):
    """Sprites a text costs: one per non-space character."""
    return sum(1 for c in text if not c.isspace())


def _candidates(root, pattern):
    root = Path(root)
    return [
        p
        for p in sorted(root.glob(pattern))
        if not (_SKIP_DIRS & set(p.relative_to(root).parts))
    ]


def find_file(root, name):
    """game/ first, then the engine core, then anywhere (build/ and .git skipped)."""
    for pattern in (f"game/**/{name}", f"extern/engine/core/**/{name}", f"**/{name}"):
        found = _candidates(root, pattern)
        if found:
            return found[0]
    return None


def read_int(root, filename, constant, default, missing=None):
    """Value of `constant` in the first `filename` found, else `default`.
    `missing`, when a list, collects "filename" for every file that was not found
    (the engine headers live in a submodule that CI may not have checked out)."""
    path = find_file(root, filename)
    if path is not None:
        match = re.search(rf"\b{constant}\s*=\s*(\d+)", path.read_text())
        if match:
            return int(match.group(1))
    if path is None and missing is not None and filename not in missing:
        missing.append(filename)
    return default


def read_controls_lines(root):
    path = find_file(root, "controls_text.h")
    if path is not None:
        match = re.search(r"LINES\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", path.read_text(), re.DOTALL)
        if match:
            lines = re.findall(r'"([^"]*)"', match.group(1))
            if lines:
                return tuple(lines)
    return DEFAULT_CONTROLS_LINES


class Budget:
    """All the numbers the checks need, read from the sources when found."""

    def __init__(self, **overrides):
        self.oam = OAM_SLOTS
        self.headroom = WARN_HEADROOM
        self.values = dict(DEFAULTS)
        self.controls_lines = DEFAULT_CONTROLS_LINES
        self.missing = []  # source files not found: their defaults are assumed
        for key, value in overrides.items():
            if key == "oam":
                self.oam = value
            elif key == "headroom":
                self.headroom = value
            elif key == "controls_lines":
                self.controls_lines = tuple(value)
            else:
                self.values[key] = value

    def __getattr__(self, name):
        try:
            return self.__dict__["values"][name]
        except KeyError:
            raise AttributeError(name) from None

    def _pause_views(self):
        """(menu rows, title) of each view of the pause overlay. Only one view
        exists at a time; each has one '>' cursor, and the controls view has
        no title at all."""
        main = sum(glyphs(r) for r in PAUSE_MAIN_ROWS) + 1
        options = sum(glyphs(r) for r in PAUSE_OPTIONS_ROWS) + 1
        # the controls view has no title (pause_controller.cpp)
        controls = sum(glyphs(r) for r in self.controls_lines)
        return ((main, PAUSE_TITLE_GLYPHS), (options, PAUSE_TITLE_GLYPHS), (controls, 0))

    @property
    def pause_menu(self):
        """Worst case of the menu rows (the _menu_sprites vector)."""
        return max(rows for rows, _ in self._pause_views())

    @property
    def pause_overlay(self):
        """Worst case of everything the pause overlay allocates while it is
        open: the rows of the widest view plus its 'Paused' title. All of it is
        released on resume."""
        return max(rows + title for rows, title in self._pause_views())

    @property
    def level_select(self):
        """Worst case of the level select: every row of the longest world
        shows both marks, the selected one also its best time, plus the goals
        and Mirror Mode lines."""
        return (
            (self.LEVEL_SELECT_ROWS - 1) * glyphs(LEVEL_SELECT_ROW)
            + glyphs(LEVEL_SELECT_SELECTED_ROW)
            + glyphs(LEVEL_SELECT_GOALS)
            + glyphs(LEVEL_SELECT_MIRROR)
        )

    @property
    def banner(self):
        return sum(glyphs(t) for t in BANNER_LINES)

    @property
    def hud(self):
        return DEATH_COUNTER_GLYPHS + TIMER_GLYPHS


def load_budget(root="."):
    missing = []
    values = {
        "MAX_SPRITES": read_int(
            root, "sprite_registry.h", "MAX_SPRITES", DEFAULTS["MAX_SPRITES"], missing
        ),
        "SLOTS": read_int(root, "dash_trail.h", "SLOTS", DEFAULTS["SLOTS"], missing),
        "MAX_STATIC_BODIES": read_int(
            root, "collision_registry.h", "MAX_STATIC_BODIES", DEFAULTS["MAX_STATIC_BODIES"], missing
        ),
        "MAX_PHYSICS_BODIES": read_int(
            root, "collision_registry.h", "MAX_PHYSICS_BODIES", DEFAULTS["MAX_PHYSICS_BODIES"], missing
        ),
        "MAX_COLLISION_SHAPES": read_int(
            root, "collision_registry.h", "MAX_COLLISION_SHAPES", DEFAULTS["MAX_COLLISION_SHAPES"], missing
        ),
    }
    world_index = find_file(root, "world_index.h")
    if world_index is not None:
        counts = re.findall(r'\{"World[^"]*",\s*\d+,\s*(\d+)\}', world_index.read_text())
        if counts:
            values["LEVEL_SELECT_ROWS"] = max(map(int, counts))
    for key in ("PLATFORMS", "PLATFORM_BODIES", "TRIGGERS", "TOTAL_TRAPS", "CHECKPOINTS", "ENTITY_SPRITES"):
        values[key] = read_int(root, "cfg.h", key, DEFAULTS[key], missing)
    budget = Budget(**values)
    budget.controls_lines = read_controls_lines(root)
    budget.missing = missing
    return budget


def hint_glyphs(level):
    return glyphs(level.hint) if level.hint else 0


def demand(level, budget, dash=True, pogo=True):
    """Worst-case sprite demand of one level.

    registry: sprites registered with SpriteRegistry.
    scenarios: total allocated sprites (OAM) in each situation.
    dash / pogo: whether the level has them (abilities()); without them the
    dash afterimages / the pogo slash never exist.
    """
    switch_sprites = sum(1 for t in level.triggers if t.get("has_sprite"))
    registry = (
        PLAYER_AND_DOOR
        + len(level.platforms)
        + len(level.traps)
        + switch_sprites
        + len(level.checkpoints)
        # dash afterimages; the death ghost takes one of these slots, since
        # death clears the trail first (one slot without the dash)
        + (budget.SLOTS if dash else 1)
        + (POGO_SLASH if pogo else 0)
    )
    fixed = registry + budget.hud
    hint = hint_glyphs(level)
    scenarios = {
        "play": fixed + hint,
        "banner": fixed + hint + budget.banner,
        # LevelScene releases the hint's sprites while the pause overlay is
        # open (hint::should_draw()), so the two never coexist.
        "pause": fixed + budget.pause_overlay,
    }
    return registry, scenarios


def abilities(root):
    """{level name: (dash, pogo)} as LevelManager::_load_player_spawn() sets
    them: from their Cfg::...::UNLOCK_WORLD on, the pogo also in every level
    with a boss. Read from main.cpp's levels[], world_index.h and the level
    JSON; {} when they cannot be found (then every level counts both)."""
    root = Path(root)
    try:
        spec = importlib.util.spec_from_file_location(
            "check_world_index", Path(__file__).with_name("Check_world_index.py"))
        cwi = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cwi)
        order = cwi.parse_levels((root / "game/src/main.cpp").read_text())
        worlds = cwi.parse_worlds((root / "game/include/world_index.h").read_text())

        def unlock(header):
            text = (root / "game/include" / header).read_text()
            return worlds[int(re.search(r"UNLOCK_WORLD\s*=\s*(\d+)", text).group(1))][1]

        dash_from, pogo_from = unlock("cfg_dash.h"), unlock("cfg_pogo.h")
        bosses = set()
        for path in sorted((root / "game/levels").glob("*.json")):
            for lv in json.loads(path.read_text())["levels"]:
                traps = (lv.get("traps") or {}).get("items", [])
                if any(t.get("type") == "BOSS" for t in traps):
                    bosses.add(lv["name"])
    except (OSError, AttributeError, IndexError, KeyError, ValueError):
        return {}
    return {name: (i >= dash_from, i >= pogo_from or name in bosses)
            for i, name in enumerate(order)}


def check_level(level, budget, dash=True, pogo=True):
    """Returns (errors, warnings, (registry, oam_peak, peak_scenario))."""
    errors, warnings = [], []

    def err(text):
        errors.append(f"{level.name}: {text}")

    def warn(text):
        warnings.append(f"{level.name}: {text}")

    for label, count, limit in (
        ("platforms", len(level.platforms), budget.PLATFORMS),
        ("traps", len(level.traps), budget.TOTAL_TRAPS),
        ("triggers", len(level.triggers), budget.TRIGGERS),
        ("checkpoints", len(level.checkpoints), budget.CHECKPOINTS),
        (
            "platform, trap, checkpoint and switch sprites together",
            len(level.platforms) + len(level.traps) + len(level.checkpoints)
            + sum(1 for t in level.triggers if t.get("has_sprite")),
            budget.ENTITY_SPRITES,
        ),
    ):
        if count > limit:
            err(f"{count} {label}, Cfg::Level::Limits allows {limit}")

    registry, scenarios = demand(level, budget, dash, pogo)
    if registry > budget.MAX_SPRITES:
        err(
            f"{registry} registered sprites, SpriteRegistry holds {budget.MAX_SPRITES} "
            "(the rest would stay pinned to the screen)"
        )
    elif registry > budget.MAX_SPRITES - budget.headroom:
        warn(f"{registry} of {budget.MAX_SPRITES} registry sprites used")

    scenario = max(scenarios, key=scenarios.get)
    peak = scenarios[scenario]
    if peak > budget.oam:
        err(f"{peak} sprites allocated in the '{scenario}' situation, the hardware has {budget.oam}")
    elif peak > budget.oam - budget.headroom:
        warn(f"{peak} of {budget.oam} hardware sprites used in the '{scenario}' situation")
    return errors, warnings, (registry, peak, scenario)


def check_configuration(budget):
    """Properties of the limits themselves, independent of any level.
    Returns (errors, warnings)."""
    errors, warnings = [], []
    b = budget
    # the per-kind limits plus the combined cap (Limits::ENTITY_SPRITES) every level is held to
    entities = min(b.PLATFORMS + b.TOTAL_TRAPS + b.TRIGGERS + b.CHECKPOINTS, b.ENTITY_SPRITES)
    registry_worst = PLAYER_AND_DOOR + entities + b.SLOTS
    if registry_worst > b.MAX_SPRITES:
        warnings.append(
            f"Cfg::Level::Limits together allow {registry_worst} registered sprites "
            f"(every platform, trap, trigger and checkpoint with a sprite), SpriteRegistry holds "
            f"{b.MAX_SPRITES}: levels near the limits are not safe - this script checks every level"
        )
    # Hint and pause overlay never coexist; hint and banner can.
    oam_worst = (
        registry_worst
        + b.hud
        + max(HINT_MAX_GLYPHS + b.banner, b.pause_overlay)
    )
    if oam_worst > b.oam:
        warnings.append(
            f"the same worst case needs {oam_worst} hardware sprites with hint and banner or the "
            f"pause overlay, the hardware has {b.oam}"
        )

    if b.level_select > b.oam:
        errors.append(
            f"the level select needs {b.level_select} hardware sprites in the worst case "
            f"({b.LEVEL_SELECT_ROWS} rows with marks, one with its time, goals and Mirror lines), the hardware has {b.oam}"
        )

    shared = b.TRIGGERS + b.TOTAL_TRAPS + b.CHECKPOINTS + PLAYER_AND_DOOR
    for label, capacity, need in (
        ("MAX_STATIC_BODIES", b.MAX_STATIC_BODIES, shared + b.PLATFORM_BODIES),
        ("MAX_PHYSICS_BODIES", b.MAX_PHYSICS_BODIES, shared),
        ("MAX_COLLISION_SHAPES", b.MAX_COLLISION_SHAPES, shared),
    ):
        if capacity < need:
            errors.append(
                f"CollisionRegistry::{label} is {capacity} but Cfg::Level::Limits can need {need}"
            )
    return errors, warnings


def main(argv, root="."):
    paths = [Path(p) for p in argv] or pv._ctr.level_files(root)
    if not paths:
        print("No levels_world*.h files found - nothing to check.", file=sys.stderr)
        return 1

    budget = load_budget(root)
    can = abilities(root)
    all_errors, all_warnings, rows = [], [], []
    for path in paths:
        for level in pv.load_level_file(path).values():
            dash, pogo = can.get(level.name, (True, True))
            errors, warnings, (registry, peak, scenario) = check_level(level, budget, dash, pogo)
            all_errors.extend(errors)
            all_warnings.extend(warnings)
            rows.append((level.name, len(level.platforms), len(level.traps), registry, peak, scenario))

    cfg_errors, cfg_warnings = check_configuration(budget)
    all_errors.extend(cfg_errors)
    all_warnings.extend(cfg_warnings)

    width = max((len(r[0]) for r in rows), default=5)
    print(
        f"{'level':<{width}}  plat trap  registry/{budget.MAX_SPRITES}  "
        f"peak/{budget.oam} (situation)"
    )
    for name, plats, traps, registry, peak, scenario in rows:
        print(f"{name:<{width}}  {plats:4d} {traps:4d}  {registry:8d}      {peak:4d} ({scenario})")

    if budget.missing:
        print(
            f"NOTE  not found: {', '.join(sorted(budget.missing))} - using the built-in "
            "defaults for their constants (engine headers need the extern/engine submodule)"
        )
    for w in all_warnings:
        print(f"WARN  {w}")
    for e in all_errors:
        print(f"ERROR {e}", file=sys.stderr)
    print(
        f"\nChecked {len(rows)} level(s) in {len(paths)} file(s): "
        f"{len(all_errors)} error(s), {len(all_warnings)} warning(s)."
    )
    return 1 if all_errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
