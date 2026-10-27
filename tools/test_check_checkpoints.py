"""Unit tests for tools/Check_checkpoints.py.

A synthetic level file with one level per rule, then the real level files.
Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_checkpoints.py")
_spec = importlib.util.spec_from_file_location("check_checkpoints", _SCRIPT)
cc = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(cc)

COMMON = """
constexpr bn::fixed_point ROUTE[] = {bn::fixed_point(0, 0), bn::fixed_point(40, 0)};
constexpr bn::fixed_point TRI[] = {
    bn::fixed_point(0, 0), bn::fixed_point(40, 0), bn::fixed_point(40, -40)};
"""

HEAD = """
constexpr PlatformData plats[] = {
    {-40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {160, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {300, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {-120, 60, 32, 16, 0, 0, bn::sprite_items::p, 0, "sw", true}};

constexpr TriggerData trigs[] = {{-120, 30, 16, 16, false, "sw", TriggerMode::Toggle}};
"""

BASE = "{TrapType::BASE, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0}"
PATH = ("{TrapType::PATH, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
        " 0, 0, 0, ROUTE, 15, 0, 0, \"sw\"}")
PATH_TRI = ("{TrapType::PATH, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
            " 0, 0, 0, TRI, 15, 0, 0, \"sw\"}")
MOVING = ("{TrapType::MOVING, %d, %d, 16, 16, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
          " 0.0, %s, %s, NO_PATH_TRAP, 0, 0, 0, \"sw\"}")
CHASE = ("{TrapType::CHASE, %d, %d, 32, 16, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
         " 0, 0, 0, NO_PATH_TRAP, 0, 40, 2.5}")
CYCLE = ("{TrapType::CYCLE, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
         " 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, nullptr, 0, 0, 0, 0, %d, 8, 40, 8, 60, 0}")
AMBUSH = ("{TrapType::AMBUSH, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
          " 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, nullptr, %d, 1, 5}")


CRUMBLE_PLATS = """
constexpr PlatformData plats_crumble[] = {
    {-40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0, nullptr, true, 30, 0}};
"""

# Level Devil: the platform at 40 is only there for the first 3 attempts.
WINDOW_PLATS = """
constexpr PlatformData plats_window[] = {
    {-40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0, nullptr, true, 0, 0, {}, 0, 0, 0, 3},
    {110, 60, 32, 16, 0, 0, bn::sprite_items::p, 0}};
"""


def level(name, cps, traps=(), plats_ref="plats, 5"):
    traps_text = ""
    trap_ref = "nullptr, 0"
    if traps:
        traps_text = f"constexpr TrapData {name}_traps[] = {{{', '.join(traps)}}};\n"
        trap_ref = f"{name}_traps, {len(traps)}"
    cps_text = "constexpr CheckpointData %s_cps[] = {%s};\n" % (
        name, ", ".join("{%d, %d}" % c for c in cps))
    return (
        traps_text + cps_text
        + f"const LevelData {name} = {{{plats_ref}, trigs, 1, {trap_ref}, "
        "bn::regular_bg_items::w, bn::music_items::m, {-40, 30}, {40, 40}, 640, 160, 1, nullptr, "
        f"{name}_cps, {len(cps)}}};\n"
    )


LEVELS = HEAD + CRUMBLE_PLATS + WINDOW_PLATS + "".join(
    [
        level("WINDOW_GONE", [(40, 44)], plats_ref="plats_window, 3"),
        level("WINDOW_REACH", [(110, 44)], plats_ref="plats_window, 3"),
        level("WINDOW_OK", [(-40, 44)], plats_ref="plats_window, 3"),
        level("CRUMBLE", [(40, 44)], plats_ref="plats_crumble, 2"),
        level("CRUMBLE_OK", [(-40, 44)], plats_ref="plats_crumble, 2"),
        level("OK", [(-40, 44), (40, 44)]),
        level("FLOAT", [(-40, 30)]),
        level("OUTSIDE", [(700, 44)]),
        level("SWITCHED", [(-120, 44)]),
        level("INSIDE", [(-40, 56)]),
        level("ONTRAP", [(40, 44)], [BASE % (40, 44)]),
        level("ONROUTE", [(40, 44)], [PATH % (0, 44)]),
        # between the two nodes of the route (4,44) -> (44,44), clear of both
        level("ON_SEGMENT", [(24, 44)], [PATH % (4, 44)]),
        # only the segment that closes the loop (60,24) -> (20,64) passes it
        level("ON_CLOSING_SEGMENT", [(40, 44)], [PATH_TRI % (20, 64)]),
        # a falling trap's track passes through the flag long after its start
        level("MOVING_FALLS", [(40, 44)], [MOVING % (40, -60, "4", "4")]),
        level("MOVING_GOES_UP", [(40, 44)], [MOVING % (40, -60, "-4", "4")]),
        level("MOVING_STANDS_STILL", [(40, 44)], [MOVING % (40, -60, "4", "0")]),
        # a chaser sweeps its row to the right and never back
        level("CHASE_AHEAD", [(40, 44)], [CHASE % (-200, 44)]),
        level("CHASE_OTHER_ROW", [(40, 44)], [CHASE % (-200, -20)]),
        level("CHASE_BEHIND", [(40, 44)], [CHASE % (100, 44)]),
        # spikes that rise out of the floor under the flag: the whole stroke
        # counts, not only the resting place
        level("CYCLE_STROKE", [(40, 44)], [CYCLE % (40, 90, -50)]),
        level("CYCLE_CLEAR", [(40, 44)], [CYCLE % (40, 120, -20)]),
        level("AMBUSH_NEAR", [(40, 44)], [AMBUSH % (70, 56, 40)]),
        level("AMBUSH_HIT", [(40, 44)], [AMBUSH % (46, 44, 40)]),
        level("UNREACHABLE", [(300, 44)]),
        level("DASHONLY", [(160, 44)]),
        level("ORDER", [(40, 44), (-40, 44)]),
        level("CLOSE", [(-40, 44), (-30, 44)]),
        level("TOOMANY", [(-40, 44)] * 5),
        level("NONE", []),
    ]
)


class SyntheticTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        Path(cls.tmp.name, "levels_common.h").write_text(COMMON)
        path = Path(cls.tmp.name, "levels_worldC.h")
        path.write_text(LEVELS)
        cls.levels = cc.pv.load_level_file(path)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_check(self, name, **kwargs):
        # the synthetic levels use several flags, so the default limit of one
        # per level would only get in the way of what they test
        kwargs.setdefault("limit", 4)
        return cc.check_level(self.levels[name], **kwargs)

    def assertError(self, name, fragment, **kwargs):
        errors, _ = self.run_check(name, **kwargs)
        self.assertTrue(any(fragment in e for e in errors), (name, errors))

    def assertWarning(self, name, fragment):
        errors, warnings = self.run_check(name)
        self.assertEqual(errors, [], name)
        self.assertTrue(any(fragment in w for w in warnings), (name, warnings))

    def test_valid_checkpoints_are_clean(self):
        self.assertEqual(self.run_check("OK"), ([], []))

    def test_level_without_checkpoints_is_clean(self):
        self.assertEqual(self.run_check("NONE"), ([], []))

    def test_floating_flag(self):
        self.assertError("FLOAT", "does not stand on a platform")

    def test_outside_the_world(self):
        self.assertError("OUTSIDE", "outside the world")

    def test_switched_platform_is_no_support(self):
        self.assertError("SWITCHED", "switched by 'sw'")

    def test_a_crumbling_platform_is_no_support(self):
        self.assertError("CRUMBLE", "crumbles after 30 frames")

    def test_the_other_platform_of_a_crumbling_level_is_fine(self):
        errors, _ = self.run_check("CRUMBLE_OK")
        self.assertEqual(errors, [])

    def test_a_platform_that_is_not_there_on_every_attempt_is_no_support(self):
        self.assertError("WINDOW_GONE", "only there after 0 .. 2 deaths")

    def test_the_support_must_be_reachable_on_every_attempt(self):
        self.assertError("WINDOW_REACH", "cannot be reached from the spawn after 3 deaths")

    def test_an_always_there_support_in_a_level_with_windows_is_fine(self):
        errors, _ = self.run_check("WINDOW_OK")
        self.assertEqual(errors, [])

    def test_respawn_inside_a_platform(self):
        self.assertError("INSIDE", "overlaps platform")

    def test_respawn_on_a_trap(self):
        self.assertError("ONTRAP", "overlaps a BASE trap")

    def test_respawn_on_a_path_route_node(self):
        self.assertError("ONROUTE", "overlaps a PATH trap")

    def test_a_flag_between_two_route_nodes_is_on_the_path(self):
        self.assertError("ON_SEGMENT", "overlaps a PATH trap's path")

    def test_the_segment_that_closes_the_loop_is_part_of_the_path(self):
        self.assertError("ON_CLOSING_SEGMENT", "overlaps a PATH trap's path")

    def test_the_whole_track_of_a_moving_trap_is_part_of_its_path(self):
        self.assertError("MOVING_FALLS", "overlaps a MOVING trap's path")

    def test_a_moving_trap_that_leaves_the_flag_behind_is_fine(self):
        for name in ("MOVING_GOES_UP", "MOVING_STANDS_STILL"):
            errors, _ = self.run_check(name)
            self.assertEqual(errors, [], name)

    def test_the_row_a_chaser_sweeps_to_the_right_is_part_of_its_path(self):
        self.assertError("CHASE_AHEAD", "overlaps a CHASE trap's path")

    def test_a_flag_in_another_row_or_behind_a_chaser_is_fine(self):
        # the chaser only ever moves right: a flag to its left is never reached
        for name in ("CHASE_OTHER_ROW", "CHASE_BEHIND"):
            errors, _ = self.run_check(name)
            self.assertEqual(errors, [], name)

    def test_a_base_trap_is_called_a_trap_and_a_moving_one_its_path(self):
        base, _ = self.run_check("ONTRAP")
        self.assertTrue(any("overlaps a BASE trap at" in e for e in base), base)

    def test_the_whole_stroke_of_a_cycle_trap_is_part_of_its_path(self):
        self.assertError("CYCLE_STROKE", "overlaps a CYCLE trap's path")

    def test_a_cycle_trap_that_never_reaches_the_flag_is_fine(self):
        errors, _ = self.run_check("CYCLE_CLEAR")
        self.assertEqual(errors, [])

    def test_ambush_range_is_a_warning(self):
        self.assertWarning("AMBUSH_NEAR", "inside the ambush range")

    def test_respawn_inside_an_ambush_lunge_is_an_error(self):
        self.assertError("AMBUSH_HIT", "overlaps a AMBUSH trap")

    def test_unreachable_platform(self):
        self.assertError("UNREACHABLE", "cannot be reached from the spawn")

    def test_dash_only_platform_is_a_warning(self):
        self.assertWarning("DASHONLY", "only reachable with the dash")

    def test_reachability_can_be_skipped(self):
        errors, warnings = self.run_check("UNREACHABLE", reach=False)
        self.assertEqual((errors, warnings), ([], []))

    def test_order_and_spacing_warnings(self):
        self.assertWarning("ORDER", "left-to-right")
        self.assertWarning("CLOSE", "closer than")

    def test_too_many_checkpoints(self):
        self.assertError("TOOMANY", "5 checkpoints, at most 4", limit=4)
        errors, _ = self.run_check("TOOMANY", limit=5)
        self.assertFalse(any("at most" in e for e in errors))

    def test_limit_is_read_from_cfg_h(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(cc.max_checkpoints(tmp), cc.DEFAULT_MAX)
            Path(tmp, "cfg.h").write_text("inline constexpr unsigned int CHECKPOINTS = 7;")
            self.assertEqual(cc.max_checkpoints(tmp), 7)

    def test_main_exit_codes(self):
        import contextlib, io

        def run(*args):
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                return cc.main([str(a) for a in args])

        ok = Path(self.tmp.name, "levels_worldOK.h")
        ok.write_text(HEAD + level("ONLY_OK", [(-40, 44)]))
        bad = Path(self.tmp.name, "levels_worldBAD.h")
        bad.write_text(HEAD + level("ONLY_BAD", [(-40, 30)]))
        self.assertEqual(run(ok), 0)
        self.assertEqual(run(bad), 1)


# A BASE trap only there from `start` deaths on (until `end`): TrapData's
# from_death / until_death come after the fields the other literals set.
BASE_WINDOW = ("{TrapType::BASE, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
               " 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,"
               " 0, false, %d, %d}")

# A patrol without a trigger: it runs from the first frame.
PATH_FREE = ("{TrapType::PATH, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
             " 0, 0, 0, ROUTE, 15, 0, 0, nullptr}")

# Level Devil: a stone drops onto the spawn (-40, 30) after 2 deaths.
SPAWN_PLATS = """
constexpr PlatformData plats_spawn[] = {
    {-40, 60, 32, 16, 0, 0, bn::sprite_items::p, 0},
    {-40, 30, 16, 8, 0, 0, bn::sprite_items::p, 0, nullptr, true, 0, 0, {}, 0, 0, 2}};
"""

SPAWN_LEVELS = HEAD + SPAWN_PLATS + "".join(
    [
        # the spawn is (-40, 30); the duck falls from there onto the platform top at y=52
        level("SPAWN_CLEAR", [], [BASE % (40, 44)]),
        level("SPAWN_TRAP_LATER", [], [BASE_WINDOW % (-40, 30, 3, 0)]),
        level("SPAWN_TRAP_FIRST", [], [BASE_WINDOW % (-40, 30, 0, 2)]),
        level("SPAWN_LANDING_TRAP", [], [BASE % (-40, 46)]),
        level("SPAWN_ON_ROUTE", [], [PATH_FREE % (-60, 40)]),
        level("SPAWN_GATED_ROUTE", [], [PATH % (-60, 40)]),
        level("SPAWN_CYCLE_STROKE", [], [CYCLE % (-40, 90, -50)]),
        level("SPAWN_CHASER_BEHIND", [], [CHASE % (-200, 44)]),
        level("SPAWN_STONE_LATER", [], plats_ref="plats_spawn, 2"),
    ]
)


class SpawnTests(unittest.TestCase):
    """The level spawn must be clear of every trap and platform on every
    attempt: a `from_death: 3` trap on it kills on every respawn after the
    third death."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        Path(cls.tmp.name, "levels_common.h").write_text(COMMON)
        path = Path(cls.tmp.name, "levels_worldS.h")
        path.write_text(SPAWN_LEVELS)
        cls.levels = cc.pv.load_level_file(path)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def errors(self, name):
        return cc.check_spawn(self.levels[name])

    def assertSpawnError(self, name, fragment):
        errors = self.errors(name)
        self.assertTrue(any(fragment in e for e in errors), (name, errors))

    def test_a_spawn_clear_of_every_trap_is_fine(self):
        self.assertEqual(self.errors("SPAWN_CLEAR"), [])

    def test_a_trap_that_appears_on_the_spawn_after_deaths_is_an_error(self):
        self.assertSpawnError("SPAWN_TRAP_LATER", "the spawn overlaps a BASE trap at (-40,30) after 3 deaths")

    def test_a_trap_on_the_spawn_on_the_first_attempts_is_an_error(self):
        self.assertSpawnError("SPAWN_TRAP_FIRST", "the spawn overlaps a BASE trap at (-40,30)")
        self.assertFalse(any("after" in e for e in self.errors("SPAWN_TRAP_FIRST")))

    def test_the_fall_from_the_spawn_to_the_floor_counts(self):
        self.assertSpawnError("SPAWN_LANDING_TRAP", "the spawn overlaps a BASE trap at (-40,46)")

    def test_a_patrol_route_over_the_spawn_is_an_error(self):
        self.assertSpawnError("SPAWN_ON_ROUTE", "overlaps a PATH trap's path")

    def test_a_patrol_that_waits_for_its_trigger_only_counts_where_it_starts(self):
        # the duck has long left the spawn when the patrol it guards starts
        self.assertEqual(self.errors("SPAWN_GATED_ROUTE"), [])

    def test_a_cycle_stroke_through_the_spawn_is_an_error(self):
        self.assertSpawnError("SPAWN_CYCLE_STROKE", "overlaps a CYCLE trap's path")

    def test_a_chaser_behind_the_spawn_is_its_job_not_an_error(self):
        # it hunts the duck by design: only where it starts counts
        self.assertEqual(self.errors("SPAWN_CHASER_BEHIND"), [])

    def test_a_platform_that_appears_on_the_spawn_after_deaths_is_an_error(self):
        self.assertSpawnError("SPAWN_STONE_LATER", "the spawn overlaps platform 1 after 2 deaths")

    def test_main_reports_a_bad_spawn_in_a_level_without_checkpoints(self):
        import contextlib, io

        bad = Path(self.tmp.name, "levels_worldSPAWNBAD.h")
        bad.write_text(HEAD + level("ONLY_SPAWN_BAD", [], [BASE_WINDOW % (-40, 30, 3, 0)]))
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(cc.main([str(bad)]), 1)


def _real_files():
    here = Path(__file__).resolve().parent
    return sorted(here.glob("levels_world*.h")) or sorted(
        here.parent.glob("game/**/levels_world*.h")
    )


@unittest.skipIf(not _real_files(), "levels_world*.h not found")
class RealLevelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.levels = {}
        for file in _real_files():
            cls.levels.update(cc.pv.load_level_file(file))

    def test_every_real_checkpoint_is_placed_correctly(self):
        # reach=False keeps this fast; the dash level below also checks reachability
        for name, lv in self.levels.items():
            errors, _ = cc.check_level(lv, reach=False)
            self.assertEqual(errors, [], name)

    def test_dash_level_checkpoint_is_reachable_and_flagged_as_dash_only(self):
        errors, warnings = cc.check_level(self.levels["LEVEL_WORLD5_DASH"])
        self.assertEqual(errors, [])
        self.assertTrue(any("only reachable with the dash" in w for w in warnings))

    def test_a_boss_checkpoint_is_reachable(self):
        self.assertEqual(len(self.levels["LEVEL_WORLD3_BOSS"].checkpoints), 1)
        errors, _ = cc.check_level(self.levels["LEVEL_WORLD3_BOSS"])
        self.assertEqual(errors, [])

    def test_every_level_has_at_most_one_checkpoint(self):
        root = Path(__file__).resolve().parent.parent
        self.assertEqual(cc.max_checkpoints(root), 1)
        for name, lv in self.levels.items():
            self.assertLessEqual(len(lv.checkpoints), 1, name)

    def test_no_real_checkpoint_is_in_a_trap_or_on_its_path(self):
        for name, lv in self.levels.items():
            errors, _ = cc.check_level(lv, reach=False)
            self.assertEqual([e for e in errors if "trap" in e], [], name)

    def test_the_two_charging_boss_floors_keep_their_flag_in_the_arena(self):
        # World 1 and 2 bosses charge a trap along the whole floor, so no spot
        # there is clear of a trap's path: their one flag stands on the boss
        # arena's raised left ledge, above that floor.
        for name in ("LEVEL_WORLD1_BOSS", "LEVEL_WORLD2_BOSS"):
            lv = self.levels[name]
            self.assertEqual(len(lv.checkpoints), 1, name)
            floor_top = min(p.top for p in lv.platforms if p.top > 40)
            self.assertLess(lv.checkpoints[0][1], floor_top - 20, name)

    def test_no_real_spawn_is_in_a_trap_on_any_attempt(self):
        for name, lv in self.levels.items():
            self.assertEqual(cc.check_spawn(lv), [], name)

    def test_limit_in_cfg_h_matches_the_data(self):
        limit = cc.max_checkpoints(Path(__file__).resolve().parent)
        for name, lv in self.levels.items():
            self.assertLessEqual(len(lv.checkpoints), limit, name)


if __name__ == "__main__":
    unittest.main()
