"""Unit tests for tools/Preview_level.py.

A synthetic level file pins down the drawing (coordinates, classes, arcs,
trap shapes); the real level files are rendered as a smoke test. Run from the
repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import contextlib
import importlib.util
import io
import re
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Preview_level.py")
_spec = importlib.util.spec_from_file_location("preview_level", _SCRIPT)
pv = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(pv)

SVG = "{http://www.w3.org/2000/svg}"

COMMON = """
constexpr bn::fixed_point MY_PATH[] = {
    bn::fixed_point(0, 0), bn::fixed_point(20, 0), bn::fixed_point(20, 10)};
"""

LEVELS = """
constexpr PlatformData plats[] = {
    {-40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {160, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {400, 60, 32, 16, 0, 0, bn::sprite_items::p, 0}};

constexpr TriggerData trigs[] = {
    {0, 30, 20, 20, false, "go"},
    {100, 30, 10, 10, false, "lev", TriggerMode::Toggle},
    {200, 30, 10, 10, false, "slow", TriggerMode::Latch, nullptr, 0, 1, 45},
    {220, 30, 10, 10, false, "clock", TriggerMode::Timer, nullptr, 0, 1, 120},
    {240, 30, 10, 10, false, "hop", TriggerMode::Toggle, nullptr, 0, 1, 0,
     TriggerAction::Jump},
    {260, 30, 10, 10, false, "zip", TriggerMode::Latch, nullptr, 0, 1, 15,
     TriggerAction::Dash}};

constexpr TrapData traps[] = {
    {TrapType::BASE, 0, 40, 12, 12, 0, 0, bn::sprite_items::t, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},
    {TrapType::MOVING, 0, -40, 16, 16, 0, 0, bn::sprite_items::t, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "go"},
    {TrapType::PATH, 10, 10, 16, 16, 0, 0, bn::sprite_items::t, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, MY_PATH, 15, 0, 0, "go"},
    {TrapType::AMBUSH, 200, 50, 12, 12, 0, 0, bn::sprite_items::t, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, nullptr, 40, 1.5,
     20},
    {TrapType::CHASE, -100, 20, 12, 12, 0, 0, bn::sprite_items::t, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 40, 2.5}};

constexpr CheckpointData cps[] = {{-40, 44}, {40, 44}};

const LevelData LEVEL_T = {
    plats, sizeof(plats) / sizeof(plats[0]),
    trigs, sizeof(trigs) / sizeof(trigs[0]),
    traps, sizeof(traps) / sizeof(traps[0]),
    bn::regular_bg_items::w, bn::music_items::m, {-40, 30}, {40, 40},
    640, 160, 1, "hello", cps, 2};

const LevelData LEVEL_T2 = {
    plats, sizeof(plats) / sizeof(plats[0]),
    nullptr, 0, nullptr, 0,
    bn::regular_bg_items::w, bn::music_items::m, {-40, 30}, {40, 40}};
"""


def make_files(tmp):
    Path(tmp, "levels_common.h").write_text(COMMON)
    path = Path(tmp, "levels_worldT.h")
    path.write_text(LEVELS)
    return path


def children(root, cls):
    return [e for e in root.iter(SVG + "rect") if cls in e.get("class", "").split()]


def rects_with(root, *classes):
    return [
        e
        for e in root.iter(SVG + "rect")
        if all(c in e.get("class", "").split() for c in classes)
    ]


def drawn(root, *classes):
    """Like rects_with, but only real level objects (legend swatches have no
    <title>)."""
    return [r for r in rects_with(root, *classes) if title_of(r)]


def title_of(element):
    node = element.find(SVG + "title")
    return node.text if node is not None else ""


class Fixture(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.path = make_files(cls.tmp.name)
        cls.levels = pv.load_level_file(cls.path)
        cls.level = cls.levels["LEVEL_T"]
        cls.cache = {}

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def svg(self, **kwargs):
        key = tuple(sorted(kwargs.items()))
        if key not in self.cache:
            self.cache[key] = ET.fromstring(pv.render(self.level, **kwargs))
        return self.cache[key]


class ParsingTests(Fixture):
    def test_both_levels_are_found(self):
        self.assertEqual(sorted(self.levels), ["LEVEL_T", "LEVEL_T2"])

    def test_world_size_hint_spawn_and_door(self):
        self.assertEqual((self.level.world_w, self.level.world_h), (640.0, 160.0))
        self.assertEqual(self.level.hint, "hello")
        self.assertEqual(self.level.spawn, (-40.0, 30.0))
        self.assertEqual(self.level.door, (40.0, 40.0))

    def test_default_world_size_is_one_screen(self):
        other = self.levels["LEVEL_T2"]
        self.assertEqual((other.world_w, other.world_h), (240.0, 160.0))

    def test_trap_fields(self):
        kinds = [t["type"] for t in self.level.traps]
        self.assertEqual(kinds, ["BASE", "MOVING", "PATH", "AMBUSH", "CHASE"])
        moving, path, ambush, chase = (self.level.traps[i] for i in (1, 2, 3, 4))
        self.assertEqual((moving["velocity_y"], moving["max_vel"]), (4.0, 4.0))
        self.assertEqual(moving["trigger"], "go")
        self.assertEqual(path["path"], [(0.0, 0.0), (20.0, 0.0), (20.0, 10.0)])
        self.assertEqual(path["path_waits"], 15.0)
        self.assertEqual(
            (ambush["ambush_range"], ambush["ambush_speed"], ambush["ambush_duration"]),
            (40.0, 1.5, 20.0),
        )
        self.assertEqual(
            (chase["chase_follow_distance"], chase["chase_speed"]), (40.0, 2.5)
        )

    def test_checkpoints_are_parsed_in_order(self):
        self.assertEqual(self.level.checkpoints, [(-40.0, 44.0), (40.0, 44.0)])
        self.assertEqual(self.levels["LEVEL_T2"].checkpoints, [])

    def test_trigger_modes_and_names(self):
        self.assertEqual(
            [t["name"] for t in self.level.triggers],
            ["go", "lev", "slow", "clock", "hop", "zip"],
        )
        self.assertEqual(
            [t["mode"] for t in self.level.triggers],
            ["Latch", "Toggle", "Latch", "Timer", "Toggle", "Latch"],
        )

    def test_trigger_delay_is_parsed_and_defaults_to_zero(self):
        self.assertEqual([t["delay"] for t in self.level.triggers], [0, 0, 45, 120, 0, 15])

    def test_trigger_action_is_parsed_and_defaults_to_enter(self):
        self.assertEqual(
            [t["action"] for t in self.level.triggers],
            ["Enter", "Enter", "Enter", "Enter", "Jump", "Dash"],
        )

    def test_trigger_note_names_delay_and_timer(self):
        notes = [pv.trigger_note(t) for t in self.level.triggers]
        self.assertEqual(
            notes,
            ["", " [Toggle]", " +45f", " [Timer 120f]", " [Toggle] on jump", " on dash +15f"],
        )


class GeometryTests(Fixture):
    def test_moving_track_accelerates_then_clamps(self):
        trap = self.level.traps[1]
        track = pv.moving_track(trap, frames=4)
        self.assertEqual(track[0], (0.0, -40.0))
        # velocity 4 from the first frame (accel 4, max 4): 4 px per frame
        self.assertEqual(track[3], (0.0, -28.0))

    def test_moving_track_with_zero_max_vel_never_moves(self):
        trap = dict(self.level.traps[1], max_vel=0.0)
        self.assertEqual(set(pv.moving_track(trap, frames=10)), {(0.0, -40.0)})

    def test_trap_position_adds_offsets_and_first_path_node(self):
        trap = {"type": "BASE", "x": 10, "y": 20, "offset_x": 3, "offset_y": -2, "path": []}
        self.assertEqual(pv.trap_position(trap), (13, 18))
        routed = dict(trap, type="PATH", path=[(5.0, 6.0), (9.0, 9.0)])
        self.assertEqual(pv.trap_position(routed), (18, 24))


class DrawingTests(Fixture):
    def test_is_well_formed_svg_with_the_world_inside_the_view_box(self):
        root = self.svg(arcs="none")
        min_x, min_y, width, height = map(float, root.get("viewBox").split())
        self.assertLessEqual(min_x, -320)
        self.assertGreaterEqual(min_x + width, 320)
        self.assertLessEqual(min_y, -80)
        self.assertGreaterEqual(min_y + height, pv.DEATH_Y)

    def test_header_names_the_level_and_shows_the_hint(self):
        text = " ".join(t.text or "" for t in self.svg(arcs="none").iter(SVG + "text"))
        self.assertIn("LEVEL_T", text)
        self.assertIn("hello", text)
        self.assertIn("640x160", text)

    def test_platforms_use_game_coordinates_y_down(self):
        root = self.svg(arcs="none")
        plats = [r for r in root.iter(SVG + "rect") if title_of(r).startswith("platform")]
        self.assertEqual(len(plats), 4)
        first = plats[0]
        # center (-40, 60), 32x16 -> top-left (-56, 52)
        self.assertEqual(
            (float(first.get("x")), float(first.get("y"))), (-56.0, 52.0)
        )

    def test_platforms_are_coloured_by_reachability(self):
        root = self.svg(arcs="none")
        classes = {
            int(re.match(r"platform (\d+)", title_of(r)).group(1)): r.get("class").split()
            for r in root.iter(SVG + "rect")
            if title_of(r).startswith("platform")
        }
        for index in (0, 1):
            self.assertNotIn("dash-only", classes[index])
            self.assertNotIn("unreachable", classes[index])
        self.assertIn("dash-only", classes[2])  # 88px gap: dash only
        self.assertIn("unreachable", classes[3])  # far beyond even the dash

    def test_without_reachability_nothing_is_flagged(self):
        root = ET.fromstring(pv.render(self.level, arcs="none", reach=False))
        self.assertEqual(drawn(root, "unreachable"), [])
        self.assertEqual(drawn(root, "dash-only"), [])

    def test_every_trap_type_is_drawn_once(self):
        root = self.svg(arcs="none")
        for kind in ("base", "moving", "path", "ambush", "chase"):
            drawn = [
                r
                for r in rects_with(root, "trap", f"trap-{kind}")
                if title_of(r)  # the legend swatches have no title
            ]
            self.assertEqual(len(drawn), 1, kind)

    def test_delayed_and_timer_triggers_are_labelled(self):
        text = " ".join(t.text or "" for t in self.svg(arcs="none").iter(SVG + "text"))
        self.assertIn("slow +45f", text)
        self.assertIn("clock [Timer 120f]", text)
        self.assertIn("hop [Toggle] on jump", text)
        self.assertIn("zip on dash +15f", text)

    def test_triggers_are_linked_to_their_traps_and_labelled(self):
        root = self.svg(arcs="none")
        links = [e for e in root.iter(SVG + "line") if e.get("class") == "link"]
        self.assertEqual(len(links), 2)  # MOVING and PATH are bound to "go"
        text = " ".join(t.text or "" for t in root.iter(SVG + "text"))
        self.assertIn("lev [Toggle]", text)

    def test_ambush_band_and_path_route(self):
        root = self.svg(arcs="none")
        bands = rects_with(root, "band")
        self.assertEqual(len(bands), 1)
        self.assertEqual(float(bands[0].get("width")), 80.0)  # 2 * range 40
        routes = [e for e in root.iter(SVG + "polygon") if e.get("class") == "route"]
        self.assertEqual(len(routes), 1)
        self.assertEqual(len(routes[0].get("points").split()), 3)

    def test_moving_trap_leaves_ghosts_but_stays_inside_the_world(self):
        root = self.svg(arcs="none")
        ghosts = rects_with(root, "ghost")
        self.assertGreaterEqual(len(ghosts), 2)
        limit = 80 + pv.TRACK_MARGIN + 8  # half height + margin + half box
        for g in ghosts:
            self.assertLessEqual(float(g.get("y")) + float(g.get("height")) / 2, limit + 8)

    def test_checkpoints_are_drawn_where_the_duck_respawns(self):
        root = self.svg(arcs="none")
        flags = drawn(root, "checkpoint")
        self.assertEqual(len(flags), 2)
        # center (-40, 44), 16x16 -> top-left (-48, 36)
        self.assertEqual((float(flags[0].get("x")), float(flags[0].get("y"))), (-48.0, 36.0))
        text = " ".join(t.text or "" for t in root.iter(SVG + "text"))
        self.assertIn("checkpoint 2", text)
        self.assertIn("checkpoints 2", text)

    def test_spawn_and_door_markers(self):
        root = self.svg(arcs="none")
        spawn = rects_with(root, "spawn")[0]
        self.assertEqual(float(spawn.get("x")), -44.0)  # center -40, width 8
        self.assertEqual(len(rects_with(root, "door")), 1)

    def test_no_traps_option_hides_traps_and_triggers(self):
        root = ET.fromstring(pv.render(self.level, arcs="none", show_traps=False))
        drawn = [r for r in rects_with(root, "trap") if title_of(r)]
        self.assertEqual(drawn, [])
        self.assertEqual(rects_with(root, "trigger"), [])


class ArcTests(Fixture):
    def arcs(self, root, kind):
        return [
            e
            for e in root.iter(SVG + "polyline")
            if kind in e.get("class", "").split()
        ]

    def test_arcs_none_draws_none(self):
        root = self.svg(arcs="none")
        self.assertEqual(self.arcs(root, "arc"), [])

    def test_plain_arcs_only(self):
        root = self.svg(arcs="plain")
        self.assertGreaterEqual(len(self.arcs(root, "arc-plain")), 1)
        self.assertEqual(self.arcs(root, "arc-dash"), [])

    def test_all_adds_dash_arcs_only_where_no_plain_jump_exists(self):
        root = self.svg(arcs="all")
        dash = self.arcs(root, "arc-dash")
        self.assertGreaterEqual(len(dash), 1)
        # platform 1 -> 2 (88px) needs the dash; 0 -> 1 (48px) does not
        traces = pv.compute_arcs(self.level, True, False)
        plain_pairs = {(a, b) for (a, b, d) in traces if not d}
        dash_pairs = {(a, b) for (a, b, d) in traces if d}
        self.assertIn((0, 1), plain_pairs)
        self.assertNotIn((1, 2), plain_pairs)
        self.assertIn((1, 2), dash_pairs)

    def test_an_arc_starts_on_the_takeoff_platform_and_ends_on_the_landing_one(self):
        traces = pv.compute_arcs(self.level, False, False)
        points = traces[(0, 1, False)]
        # duck center sits half a body (4px) above the platform top (y 52)
        self.assertAlmostEqual(points[0][1], 48.0)
        self.assertLess(points[-1][1], 52.0)
        self.assertGreater(points[-1][0], points[0][0])
        self.assertTrue(min(y for _, y in points) < 48.0)  # it really jumps

    def test_unreachable_platform_has_no_arcs(self):
        traces = pv.compute_arcs(self.level, True, False)
        self.assertFalse(any(3 in (a, b) for (a, b, _) in traces))


class CommandLineTests(Fixture):
    def run_main(self, *args):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = pv.main([str(a) for a in args])
        return code, out.getvalue(), err.getvalue()

    def test_single_level_to_a_file(self):
        with tempfile.TemporaryDirectory() as out:
            target = Path(out, "one.svg")
            code, _, _ = self.run_main(self.path, "LEVEL_T", "--arcs", "none", "-o", target)
            self.assertEqual(code, 0)
            ET.parse(target)

    def test_all_levels_to_a_directory_with_an_index(self):
        with tempfile.TemporaryDirectory() as out:
            code, _, _ = self.run_main(
                self.path, "--arcs", "none", "--no-reach", "--out-dir", out
            )
            self.assertEqual(code, 0)
            for name in ("LEVEL_T", "LEVEL_T2"):
                ET.parse(Path(out, f"{name}.svg"))
            index = Path(out, "index.html").read_text()
            self.assertIn("LEVEL_T2.svg", index)

    def test_unknown_level_is_an_error(self):
        code, _, err = self.run_main(self.path, "LEVEL_NOPE", "--arcs", "none")
        self.assertEqual(code, 1)
        self.assertIn("LEVEL_NOPE", err)

    def test_output_file_needs_exactly_one_level(self):
        with tempfile.TemporaryDirectory() as out:
            code, _, _ = self.run_main(
                self.path, "--arcs", "none", "-o", Path(out, "x.svg")
            )
            self.assertEqual(code, 1)


def _real_files():
    here = Path(__file__).resolve().parent
    files = sorted(here.glob("levels_world*.h")) or sorted(
        here.parent.glob("game/**/levels_world*.h")
    )
    return files


@unittest.skipIf(not _real_files(), "levels_world*.h not found")
class RealLevelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.levels = {}
        for file in _real_files():
            cls.levels.update(pv.load_level_file(file))

    def test_all_levels_render_to_well_formed_svg_with_every_object(self):
        self.assertGreaterEqual(len(self.levels), 19)
        for name, level in self.levels.items():
            root = ET.fromstring(pv.render(level, arcs="none", reach=False))
            plats = [r for r in root.iter(SVG + "rect") if title_of(r).startswith("platform")]
            self.assertEqual(len(plats), len(level.platforms), name)
            traps = [
                r
                for r in root.iter(SVG + "rect")
                if "trap" in r.get("class", "").split() and title_of(r)
            ]
            self.assertEqual(len(traps), len(level.traps), name)
            triggers = rects_with(root, "trigger")
            self.assertEqual(len(triggers), len(level.triggers), name)
            self.assertEqual(len(drawn(root, "checkpoint")), len(level.checkpoints), name)

    def test_long_levels_and_bosses_have_checkpoints(self):
        for name in (
            "LEVEL_WORLD3_BOSS", "LEVEL_WORLD4_BOSS", "LEVEL_WORLD5_BOSS",
            "LEVEL_WORLD4_SCROLL", "LEVEL_WORLD5_SCROLL", "LEVEL_WORLD5_DASH",
        ):
            self.assertGreaterEqual(len(self.levels[name].checkpoints), 1, name)

    def test_dash_level_shows_platforms_that_need_the_dash_and_none_unreachable(self):
        level = self.levels["LEVEL_WORLD5_DASH"]
        root = ET.fromstring(pv.render(level, arcs="none"))
        self.assertGreaterEqual(len(drawn(root, "dash-only")), 1)
        self.assertEqual(drawn(root, "unreachable"), [])

    def test_lever_level_has_switched_platforms_and_toggle_triggers(self):
        level = self.levels["LEVEL_WORLD5_LEVERS"]
        root = ET.fromstring(pv.render(level, arcs="none", reach=False))
        self.assertEqual(len(drawn(root, "switched")), 4)
        text = " ".join(t.text or "" for t in root.iter(SVG + "text"))
        self.assertIn("lever1 [Toggle]", text)
        self.assertIn("-lever2", text)  # the stone lever2 takes away


if __name__ == "__main__":
    unittest.main()
