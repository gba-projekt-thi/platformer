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
AMBUSH = ("{TrapType::AMBUSH, %d, %d, 12, 12, 0, 0, bn::sprite_items::t, 1, TRAP_GRAPHICS_INDEXES_0,"
          " 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, nullptr, %d, 1, 5}")


def level(name, cps, traps=()):
    traps_text = ""
    trap_ref = "nullptr, 0"
    if traps:
        traps_text = f"constexpr TrapData {name}_traps[] = {{{', '.join(traps)}}};\n"
        trap_ref = f"{name}_traps, {len(traps)}"
    cps_text = "constexpr CheckpointData %s_cps[] = {%s};\n" % (
        name, ", ".join("{%d, %d}" % c for c in cps))
    return (
        traps_text + cps_text
        + f"const LevelData {name} = {{plats, 5, trigs, 1, {trap_ref}, "
        "bn::regular_bg_items::w, bn::music_items::m, {-40, 30}, {40, 40}, 640, 160, 1, nullptr, "
        f"{name}_cps, {len(cps)}}};\n"
    )


LEVELS = HEAD + "".join(
    [
        level("OK", [(-40, 44), (40, 44)]),
        level("FLOAT", [(-40, 30)]),
        level("OUTSIDE", [(700, 44)]),
        level("SWITCHED", [(-120, 44)]),
        level("INSIDE", [(-40, 56)]),
        level("ONTRAP", [(40, 44)], [BASE % (40, 44)]),
        level("ONROUTE", [(40, 44)], [PATH % (0, 44)]),
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

    def test_respawn_inside_a_platform(self):
        self.assertError("INSIDE", "overlaps platform")

    def test_respawn_on_a_trap(self):
        self.assertError("ONTRAP", "overlaps a BASE trap")

    def test_respawn_on_a_path_route_node(self):
        self.assertError("ONROUTE", "overlaps a PATH trap")

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
        errors, _ = cc.check_level(self.levels["LEVEL_WORLD1_BOSS"])
        self.assertEqual(errors, [])

    def test_limit_in_cfg_h_matches_the_data(self):
        limit = cc.max_checkpoints(Path(__file__).resolve().parent)
        for name, lv in self.levels.items():
            self.assertLessEqual(len(lv.checkpoints), limit, name)


if __name__ == "__main__":
    unittest.main()
