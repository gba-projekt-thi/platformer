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
    bn::regular_bg_items::w, bn::music_items::m, {0, 0}, {1, 1}};
"""

MOVING_A = (
    '    {TrapType::MOVING, 0, 0, 8, 8, 0, 0, bn::sprite_items::t, 1,'
    ' TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "a"},\n'
)


def run_check(triggers="", traps="", extra_platforms=""):
    text = HEADER % {
        "triggers": triggers,
        "traps": traps,
        "extra_platforms": extra_platforms,
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


if __name__ == "__main__":
    unittest.main()
