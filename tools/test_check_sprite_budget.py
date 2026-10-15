"""Unit tests for tools/Check_sprite_budget.py.

Synthetic level files pin down the counting rules, the real level files and
headers are checked as a smoke test. Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import contextlib
import importlib.util
import io
import re
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_sprite_budget.py")
_spec = importlib.util.spec_from_file_location("check_sprite_budget", _SCRIPT)
sb = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(sb)

COMMON = """
constexpr bn::fixed_point ROUTE[] = {bn::fixed_point(0, 0), bn::fixed_point(40, 0)};
"""


def platforms(n):
    items = ", ".join("{%d, 60, 32, 16, 0, 0, bn::sprite_items::p, 0}" % (-300 + 40 * i) for i in range(n))
    return f"constexpr PlatformData plats{n}[] = {{{items}}};\n", f"plats{n}, {n}"


def level(name, plats=3, traps=0, triggers=(), checkpoints=0, hint=None):
    """One synthetic LevelData; `triggers` is a list of 'sprite'/'plain'."""
    text, plat_ref = platforms(plats)
    trig_ref, trap_ref, cp_ref = "nullptr, 0", "nullptr, 0", "nullptr, 0"
    if triggers:
        entries = []
        for i, kind in enumerate(triggers):
            if kind == "sprite":
                entries.append(
                    '{%d, 30, 16, 16, false, "t%d", TriggerMode::Toggle, '
                    "&bn::sprite_items::lever16x16, 0, 1}" % (10 * i, i)
                )
            else:
                entries.append('{%d, 30, 16, 16, false, "t%d"}' % (10 * i, i))
        text += f"constexpr TriggerData {name}_trigs[] = {{{', '.join(entries)}}};\n"
        trig_ref = f"{name}_trigs, {len(triggers)}"
    if traps:
        entries = ", ".join(
            "{TrapType::BASE, %d, 40, 12, 12, 0, 0, bn::sprite_items::t, 1, "
            "TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0}" % (-200 + 10 * i)
            for i in range(traps)
        )
        text += f"constexpr TrapData {name}_traps[] = {{{entries}}};\n"
        trap_ref = f"{name}_traps, {traps}"
    if checkpoints:
        entries = ", ".join("{%d, 44}" % (-200 + 40 * i) for i in range(checkpoints))
        text += f"constexpr CheckpointData {name}_cps[] = {{{entries}}};\n"
        cp_ref = f"{name}_cps, {checkpoints}"
    hint_ref = f'"{hint}"' if hint else "nullptr"
    text += (
        f"const LevelData {name} = {{{plat_ref}, {trig_ref}, {trap_ref}, "
        "bn::regular_bg_items::w, bn::music_items::m, {-40, 30}, {40, 40}, 640, 160, 1, "
        f"{hint_ref}, {cp_ref}}};\n"
    )
    return text


class Fixture(unittest.TestCase):
    def load(self, *texts):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "levels_common.h").write_text(COMMON)
            path = Path(tmp, "levels_worldS.h")
            path.write_text("".join(texts))
            return sb.pv.load_level_file(path)

    def budget(self, **kw):
        return sb.Budget(**kw)


class GlyphTests(unittest.TestCase):
    def test_spaces_draw_no_sprite(self):
        self.assertEqual(sb.glyphs("Press A to continue"), 16)
        self.assertEqual(sb.glyphs("  \t "), 0)
        self.assertEqual(sb.glyphs(""), 0)

    def test_model_constants(self):
        b = sb.Budget()
        self.assertEqual(b.hud, 20)
        self.assertEqual(b.banner, 8 + 8 + 16)
        # options sub-menu (all rows + info line + cursor) is the widest view
        self.assertEqual(b.pause_overlay, 10 + 8 + 13 + 15 + 10 + 1)


class DemandTests(Fixture):
    def test_registry_counts_player_door_platforms_traps_checkpoints_ghosts(self):
        lv = self.load(level("A", plats=3, traps=2, checkpoints=1))["A"]
        b = self.budget()
        registry, _ = sb.demand(lv, b)
        self.assertEqual(registry, 2 + 3 + 2 + 1 + b.SLOTS)

    def test_only_triggers_with_a_sprite_count(self):
        levels = self.load(
            level("PLAIN", triggers=["plain", "plain"]),
            level("SWITCH", triggers=["sprite", "plain", "sprite"]),
        )
        b = self.budget()
        plain, _ = sb.demand(levels["PLAIN"], b)
        switch, _ = sb.demand(levels["SWITCH"], b)
        self.assertEqual(switch - plain, 2)

    def test_scenarios_add_hud_title_hint_banner_and_overlay(self):
        lv = self.load(level("H", plats=1, hint="hello world"))["H"]
        b = self.budget()
        registry, sc = sb.demand(lv, b)
        fixed = registry + b.hud + sb.PAUSE_TITLE_GLYPHS
        self.assertEqual(sc["play"], fixed + 10)  # "helloworld"
        self.assertEqual(sc["banner"], fixed + 10 + b.banner)
        self.assertEqual(sc["pause"], fixed + 10 + b.pause_overlay)

    def test_level_without_hint_costs_nothing_extra(self):
        lv = self.load(level("N", plats=1))["N"]
        b = self.budget()
        registry, sc = sb.demand(lv, b)
        self.assertEqual(sc["play"], registry + b.hud + sb.PAUSE_TITLE_GLYPHS)

    def test_pause_is_the_peak_with_the_default_model(self):
        lv = self.load(level("P", plats=2))["P"]
        _, peak, scenario = sb.check_level(lv, self.budget())[2]
        self.assertEqual(scenario, "pause")


class CheckTests(Fixture):
    def test_small_level_is_clean(self):
        lv = self.load(level("OK", plats=4, traps=2))["OK"]
        errors, warnings, _ = sb.check_level(lv, self.budget())
        self.assertEqual((errors, warnings), ([], []))

    def test_entity_limits_are_inclusive(self):
        levels = self.load(level("FULL", plats=32), level("OVER", plats=33))
        b = self.budget()
        full_errors, _, _ = sb.check_level(levels["FULL"], b)
        over_errors, _, _ = sb.check_level(levels["OVER"], b)
        self.assertFalse(any("platforms" in e for e in full_errors))
        self.assertTrue(any("33 platforms" in e for e in over_errors))

    def test_trap_trigger_and_checkpoint_limits(self):
        lv = self.load(level("MANY", plats=1, traps=3, triggers=["plain"] * 3, checkpoints=3))["MANY"]
        errors, _, _ = sb.check_level(lv, self.budget(TOTAL_TRAPS=2, TRIGGERS=2, CHECKPOINTS=2))
        text = " ".join(errors)
        self.assertIn("3 traps", text)
        self.assertIn("3 triggers", text)
        self.assertIn("3 checkpoints", text)

    def test_registry_overflow_is_an_error_and_names_the_consequence(self):
        lv = self.load(level("BIG", plats=10))["BIG"]
        errors, _, _ = sb.check_level(lv, self.budget(MAX_SPRITES=10))
        self.assertTrue(any("registered sprites" in e and "pinned" in e for e in errors))

    def test_registry_headroom_warning(self):
        lv = self.load(level("W", plats=3))["W"]
        registry, _ = sb.demand(lv, self.budget())
        errors, warnings, _ = sb.check_level(lv, self.budget(MAX_SPRITES=registry + 3))
        self.assertEqual(errors, [])
        self.assertTrue(any("registry sprites" in w for w in warnings))

    def test_oam_overflow_is_an_error(self):
        lv = self.load(level("O", plats=3))["O"]
        errors, _, _ = sb.check_level(lv, self.budget(oam=50))
        self.assertTrue(any("hardware has 50" in e for e in errors))

    def test_oam_headroom_warning_names_the_situation(self):
        lv = self.load(level("W2", plats=3))["W2"]
        _, peak, _ = sb.check_level(lv, self.budget())[2]
        errors, warnings, _ = sb.check_level(lv, self.budget(oam=peak + 2))
        self.assertEqual(errors, [])
        self.assertTrue(any("'pause'" in w for w in warnings))

    def test_a_full_level_warns_but_a_32_platform_level_does_not_error(self):
        lv = self.load(level("FULL", plats=32))["FULL"]
        errors, _, _ = sb.check_level(lv, self.budget())
        self.assertEqual(errors, [])


class ConfigurationTests(unittest.TestCase):
    def test_default_limits_exceed_the_registry_and_warn(self):
        errors, warnings = sb.check_configuration(sb.Budget())
        self.assertEqual(errors, [])
        self.assertTrue(any("SpriteRegistry holds" in w for w in warnings))

    def test_no_warning_when_the_registry_is_large_enough(self):
        errors, warnings = sb.check_configuration(sb.Budget(MAX_SPRITES=200, oam=400))
        self.assertEqual((errors, warnings), ([], []))

    def test_collision_registry_capacities_must_cover_the_limits(self):
        errors, _ = sb.check_configuration(
            sb.Budget(MAX_STATIC_BODIES=10, MAX_PHYSICS_BODIES=10, MAX_COLLISION_SHAPES=10)
        )
        self.assertEqual(len(errors), 3)
        self.assertTrue(all("CollisionRegistry::" in e for e in errors))

    def test_exact_capacity_is_enough(self):
        b = sb.Budget()
        shared = b.TRIGGERS + b.TOTAL_TRAPS + b.CHECKPOINTS + sb.PLAYER_AND_DOOR
        errors, _ = sb.check_configuration(
            sb.Budget(
                MAX_STATIC_BODIES=shared + b.PLATFORM_BODIES,
                MAX_PHYSICS_BODIES=shared,
                MAX_COLLISION_SHAPES=shared,
                MAX_SPRITES=500,
            )
        )
        self.assertEqual(errors, [])


class ReadingTests(unittest.TestCase):
    def test_constants_are_read_from_the_headers(self):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "sprite_registry.h").write_text("static constexpr int MAX_SPRITES = 99;")
            Path(tmp, "dash_trail.h").write_text("static constexpr int SLOTS = 6;")
            Path(tmp, "cfg.h").write_text(
                "inline constexpr unsigned int PLATFORMS = 20;\n"
                "inline constexpr unsigned int PLATFORM_BODIES = 21;\n"
                "inline constexpr unsigned int TRIGGERS = 7;\n"
                "inline constexpr unsigned int TOTAL_TRAPS = 9;\n"
                "inline constexpr unsigned int CHECKPOINTS = 3;\n"
                "inline constexpr uint16_t TRIGGER = 0x0008;\n"
            )
            Path(tmp, "collision_registry.h").write_text(
                "static constexpr int MAX_STATIC_BODIES = 50;\n"
                "static constexpr int MAX_PHYSICS_BODIES = 40;\n"
                "static constexpr int MAX_COLLISION_SHAPES = 30;\n"
            )
            Path(tmp, "controls_text.h").write_text('LINES[] = {"A Jump", "B Dash"};')
            b = sb.load_budget(tmp)
        self.assertEqual(
            (b.MAX_SPRITES, b.SLOTS, b.PLATFORMS, b.PLATFORM_BODIES, b.TRIGGERS, b.TOTAL_TRAPS, b.CHECKPOINTS),
            (99, 6, 20, 21, 7, 9, 3),
        )
        self.assertEqual((b.MAX_STATIC_BODIES, b.MAX_PHYSICS_BODIES, b.MAX_COLLISION_SHAPES), (50, 40, 30))
        self.assertEqual(b.controls_lines, ("A Jump", "B Dash"))

    def test_platform_bodies_is_not_confused_with_platforms(self):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "cfg.h").write_text("PLATFORM_BODIES = 5;\nPLATFORMS = 6;\n")
            self.assertEqual(sb.read_int(tmp, "cfg.h", "PLATFORMS", 0), 6)
            self.assertEqual(sb.read_int(tmp, "cfg.h", "PLATFORM_BODIES", 0), 5)

    def test_defaults_are_used_when_nothing_is_found(self):
        with tempfile.TemporaryDirectory() as tmp:
            b = sb.load_budget(tmp)
        self.assertEqual(b.MAX_SPRITES, sb.DEFAULTS["MAX_SPRITES"])
        self.assertIn("sprite_registry.h", b.missing)
        self.assertIn("cfg.h", b.missing)
        self.assertEqual(b.controls_lines, sb.DEFAULT_CONTROLS_LINES)

    def test_build_and_git_directories_are_skipped(self):
        with tempfile.TemporaryDirectory() as tmp:
            hidden = Path(tmp, "build")
            hidden.mkdir()
            Path(hidden, "cfg.h").write_text("PLATFORMS = 1;")
            self.assertIsNone(sb.find_file(tmp, "cfg.h"))


class CommandLineTests(Fixture):
    def run_main(self, path, root="."):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = sb.main([str(path)], root)
        return code, out.getvalue(), err.getvalue()

    def make(self, tmp, *texts):
        Path(tmp, "levels_common.h").write_text(COMMON)
        path = Path(tmp, "levels_worldC.h")
        path.write_text("".join(texts))
        return path

    def test_clean_file_exits_zero_and_prints_a_table(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, out, _ = self.run_main(self.make(tmp, level("ONLY_OK", plats=4)))
        self.assertEqual(code, 0)
        self.assertIn("ONLY_OK", out)
        self.assertIn("0 error(s)", out)

    def test_missing_headers_are_reported_as_a_note(self):
        with tempfile.TemporaryDirectory() as tmp:
            # an empty source root: none of the headers can be found
            code, out, _ = self.run_main(self.make(tmp, level("N", plats=4)), tmp)
        self.assertEqual(code, 0)
        self.assertIn("NOTE", out)
        self.assertIn("sprite_registry.h", out)

    def test_overfull_level_exits_one(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, _, err = self.run_main(self.make(tmp, level("TOO_BIG", plats=40)))
        self.assertEqual(code, 1)
        self.assertIn("TOO_BIG", err)


def _root():
    here = Path(__file__).resolve().parent
    return here.parent if (here.parent / "game").exists() else here


def _real_files():
    here = Path(__file__).resolve().parent
    return sorted(here.glob("levels_world*.h")) or sorted(here.parent.glob("game/**/levels_world*.h"))


@unittest.skipIf(not _real_files(), "levels_world*.h not found")
class RealLevelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.budget = sb.load_budget(_root())
        cls.levels = {}
        for file in _real_files():
            cls.levels.update(sb.pv.load_level_file(file))

    def test_every_real_level_fits_every_budget(self):
        for name, lv in self.levels.items():
            errors, _, _ = sb.check_level(lv, self.budget)
            self.assertEqual(errors, [], name)

    def test_the_real_configuration_has_no_errors(self):
        errors, _ = sb.check_configuration(self.budget)
        self.assertEqual(errors, [])

    def test_the_real_registry_capacity_is_used_when_the_engine_is_checked_out(self):
        # The engine headers live in the extern/engine submodule: without it
        # (a plain clone) the defaults apply and the script prints a NOTE.
        path = sb.find_file(_root(), "sprite_registry.h")
        if path is None:
            self.skipTest("sprite_registry.h not found (engine submodule not checked out)")
        self.assertIn("MAX_SPRITES", path.read_text())
        self.assertNotIn("sprite_registry.h", self.budget.missing)

    def test_pause_overlay_model_fits_the_real_sprite_vector(self):
        path = sb.find_file(_root(), "pause_controller.h")
        if path is None:
            self.skipTest("pause_controller.h not found")
        match = re.search(r"bn::vector<bn::sprite_ptr,\s*(\d+)>\s+_menu_sprites", path.read_text())
        self.assertIsNotNone(match)
        self.assertLessEqual(self.budget.pause_overlay, int(match.group(1)))

    def test_hints_are_inside_the_model_limit(self):
        for name, lv in self.levels.items():
            self.assertLessEqual(sb.hint_glyphs(lv), 28, name)


if __name__ == "__main__":
    unittest.main()
