"""Unit tests for tools/Check_trigger_references.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_trigger_references.py")
_spec = importlib.util.spec_from_file_location("check_trigger_references", _SCRIPT)
checker = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(checker)

HEADER = """
constexpr PlatformData plats[] = {
    {0, 60, 16, 8, 0, 0, bn::sprite_items::p, 0},
%(extra_platforms)s};

constexpr TriggerData trigs[] = {
%(triggers)s};

constexpr TrapData traps[] = {
%(traps)s};

const LevelData LEVEL_X = {
    plats, sizeof(plats) / sizeof(plats[0]),
    trigs, sizeof(trigs) / sizeof(trigs[0]),
    traps, sizeof(traps) / sizeof(traps[0]),
    bn::regular_bg_items::w, bn::music_items::m, {0, 0}, %(door)s};
"""

MOVING_A = (
    '    {TrapType::MOVING, 0, 0, 8, 8, 0, 0, bn::sprite_items::t, 1,'
    ' TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "a"},\n'
)


def run_check(triggers="", traps="", extra_platforms="", door="{1, 1}"):
    text = HEADER % {
        "triggers": triggers,
        "traps": traps,
        "extra_platforms": extra_platforms,
        "door": door,
    }
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "levels_worldX.h"
        path.write_text(text)
        return checker.check_file(path)


class TrapReferenceTests(unittest.TestCase):
    def test_resolved_trap_reference_is_clean(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n', traps=MOVING_A
        )
        self.assertEqual(errors, [])
        self.assertEqual(warnings, [])

    def test_dangling_trap_reference_is_an_error(self):
        errors, _ = run_check(
            triggers='    {0, 0, 8, 8, false, "other"},\n', traps=MOVING_A
        )
        self.assertEqual(len(errors), 1)
        self.assertIn('"a"', errors[0])

    def test_unreferenced_trigger_is_a_warning_not_an_error(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n    {9, 9, 8, 8, false, "unused"},\n',
            traps=MOVING_A,
        )
        self.assertEqual(errors, [])
        self.assertEqual(len(warnings), 1)
        self.assertIn('"unused"', warnings[0])

    def test_trigger_with_mode_and_sprite_fields_still_parses(self):
        errors, warnings = run_check(
            triggers=(
                '    {0, 0, 8, 8, false, "a", TriggerMode::Toggle,'
                " &bn::sprite_items::s, 0, 1},\n"
            ),
            traps=MOVING_A,
        )
        self.assertEqual(errors, [])
        self.assertEqual(warnings, [])


CYCLE_A = (
    '    {TrapType::CYCLE, 0, 0, 8, 8, 0, 0, bn::sprite_items::t, 1,'
    ' TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, "a", 0, 0, 0,'
    ' 0, -12, 8, 40, 8, 60, 0},\n'
)
CYCLE_FREE = (
    '    {TrapType::CYCLE, 0, 0, 8, 8, 0, 0, bn::sprite_items::t, 1,'
    ' TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 0, 0, nullptr, 0, 0, 0,'
    ' 0, -12, 8, 40, 8, 60, 0},\n'
)


class CycleReferenceTests(unittest.TestCase):
    def test_resolved_cycle_reference_is_clean(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a", TriggerMode::Timer, nullptr, 0, 1, 90},\n',
            traps=CYCLE_A,
        )
        self.assertEqual((errors, warnings), ([], []))

    def test_dangling_cycle_reference_is_an_error(self):
        errors, _ = run_check(
            triggers='    {0, 0, 8, 8, false, "other"},\n', traps=CYCLE_A
        )
        self.assertEqual(len(errors), 1)
        self.assertIn('"a"', errors[0])

    def test_a_cycle_trap_without_a_trigger_needs_none(self):
        errors, warnings = run_check(traps=CYCLE_FREE)
        self.assertEqual((errors, warnings), ([], []))


class PlatformReferenceTests(unittest.TestCase):
    SWITCHED = '    {16, 60, 16, 8, 0, 0, bn::sprite_items::p, 0, "sw", true},\n'

    def test_platform_without_trigger_name_is_unaffected(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n', traps=MOVING_A
        )
        self.assertEqual((errors, warnings), ([], []))

    def test_resolved_platform_reference_is_clean(self):
        errors, warnings = run_check(
            triggers=(
                '    {0, 0, 8, 8, false, "a"},\n'
                '    {20, 0, 8, 8, false, "sw", TriggerMode::Toggle},\n'
            ),
            traps=MOVING_A,
            extra_platforms=self.SWITCHED,
        )
        self.assertEqual(errors, [])
        # "sw" is used by a platform only - not reported as unreferenced.
        self.assertEqual(warnings, [])

    def test_dangling_platform_reference_is_an_error(self):
        errors, _ = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n',
            traps=MOVING_A,
            extra_platforms=self.SWITCHED,
        )
        self.assertEqual(len(errors), 1)
        self.assertIn('"sw"', errors[0])
        self.assertIn("platform", errors[0])


class DoorReferenceTests(unittest.TestCase):
    """DoorData::trap_trigger_name: the door that turns into a trap."""

    TRAPPED_DOOR = (
        '{1, 1, bn::sprite_items::door32x32, DOOR32X32_GRAPHICS_INDEXES, 8, false, {}, 0, 0, "%s"}'
    )

    def test_resolved_door_reference_is_clean(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n    {9, 9, 8, 8, false, "door_trap"},\n',
            traps=MOVING_A,
            door=self.TRAPPED_DOOR % "door_trap",
        )
        self.assertEqual(errors, [])
        self.assertEqual(warnings, [])

    def test_dangling_door_reference_is_an_error(self):
        errors, _ = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n',
            traps=MOVING_A,
            door=self.TRAPPED_DOOR % "missing",
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("door", errors[0])
        self.assertIn('"missing"', errors[0])


def follower(name, follows):
    """A trigger literal that follows `follows` (TriggerData::follows)."""
    return (
        f'    {{0, 0, 8, 8, false, "{name}", TriggerMode::Latch, nullptr, 0, 1,'
        f' 40, TriggerAction::Enter, "{follows}"}},\n'
    )


def cycle_on(name):
    return CYCLE_A.replace('"a"', f'"{name}"')


class FollowsTests(unittest.TestCase):
    """TriggerData::follows: delayed trap chains."""

    def test_a_resolved_chain_is_clean(self):
        # plate -> b -> c: the plate and b are used by their followers, c by
        # a trap.
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "plate"},\n'
            + follower("b", "plate") + follower("c", "b"),
            traps=cycle_on("c"),
        )
        self.assertEqual((errors, warnings), ([], []))

    def test_the_follows_name_is_not_a_defined_trigger(self):
        errors, _ = run_check(
            triggers=follower("b", "plate"), traps=cycle_on("plate")
        )
        self.assertTrue(any('"plate"' in e and "follows" in e for e in errors), errors)

    def test_follows_must_exist_in_the_same_level(self):
        errors, _ = run_check(
            triggers=follower("b", "nowhere"), traps=cycle_on("b")
        )
        self.assertEqual(len(errors), 1)
        self.assertIn('"nowhere"', errors[0])
        self.assertIn("follows", errors[0])

    def test_a_trigger_following_itself_is_an_error(self):
        errors, _ = run_check(triggers=follower("b", "b"), traps=cycle_on("b"))
        self.assertEqual(len(errors), 1)
        self.assertIn("itself", errors[0])

    def test_a_cycle_of_followers_is_an_error(self):
        errors, _ = run_check(
            triggers=follower("a", "c") + follower("b", "a") + follower("c", "b"),
            traps=MOVING_A,
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("cycle", errors[0])
        for name in ("a", "b", "c"):
            self.assertIn(f'"{name}"', errors[0])

    def test_a_chain_hanging_off_a_cycle_reports_the_cycle_once(self):
        errors, _ = run_check(
            triggers=follower("a", "b") + follower("b", "a") + follower("c", "a"),
            traps=MOVING_A,
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("cycle", errors[0])

    def test_two_followers_of_one_source_are_fine(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n'
            + follower("b", "a") + follower("c", "a"),
            traps=MOVING_A + cycle_on("b") + cycle_on("c"),
        )
        self.assertEqual((errors, warnings), ([], []))

    def test_a_platform_on_a_follower_is_an_error(self):
        # The reachability check takes a platform's trigger for a lever the
        # duck flips in its zone; a follower ignores the duck, so the check
        # would be wrong either way. Bind followers to traps.
        errors, _ = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n' + follower("sw", "a"),
            traps=MOVING_A,
            extra_platforms='    {16, 60, 16, 8, 0, 0, bn::sprite_items::p, 0, "sw", true},\n',
        )
        self.assertEqual(len(errors), 1, errors)
        self.assertIn('"sw"', errors[0])
        self.assertIn("follows", errors[0])

    def test_a_follower_nobody_uses_is_still_a_warning(self):
        errors, warnings = run_check(
            triggers='    {0, 0, 8, 8, false, "a"},\n' + follower("b", "a"),
            traps=MOVING_A,
        )
        self.assertEqual(errors, [])
        self.assertEqual(len(warnings), 1)
        self.assertIn('"b"', warnings[0])


class FindFilesTests(unittest.TestCase):
    """Shared lookup of game files by every checker: the known place first,
    a recursive search only as a fallback (it costs seconds on a big tree)."""

    def tree(self, tmp, *files):
        for name in files:
            path = Path(tmp) / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("")

    def test_game_file_is_found_in_game_include(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.tree(tmp, "game/include/cfg.h", "extern/x/cfg.h")
            self.assertEqual(checker.find_game_file(tmp, "cfg.h"), Path(tmp) / "game/include/cfg.h")

    def test_game_file_is_found_in_game_src(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.tree(tmp, "game/src/main.cpp")
            self.assertEqual(checker.find_game_file(tmp, "main.cpp"), Path(tmp) / "game/src/main.cpp")

    def test_game_file_next_to_the_root_is_found(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.tree(tmp, "hint_logic.h")
            self.assertEqual(checker.find_game_file(tmp, "hint_logic.h"), Path(tmp) / "hint_logic.h")

    def test_game_file_elsewhere_is_found_by_the_fallback_search(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.tree(tmp, "other/deep/cfg.h")
            self.assertEqual(checker.find_game_file(tmp, "cfg.h"), Path(tmp) / "other/deep/cfg.h")

    def test_missing_game_file_is_none(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertIsNone(checker.find_game_file(tmp, "cfg.h"))

    def test_level_files_come_from_game_include_level(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.tree(
                tmp,
                "game/include/level/levels_world2.h",
                "game/include/level/levels_world1.h",
                "build/copy/levels_world1.h",
            )
            level = Path(tmp) / "game/include/level"
            self.assertEqual(
                checker.level_files(tmp), [level / "levels_world1.h", level / "levels_world2.h"]
            )

    def test_level_files_fall_back_to_a_search(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.tree(tmp, "x/levels_world1.h")
            self.assertEqual(checker.level_files(tmp), [Path(tmp) / "x/levels_world1.h"])


if __name__ == "__main__":
    unittest.main()
