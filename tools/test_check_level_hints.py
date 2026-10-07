"""Unit tests for tools/Check_level_hints.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_level_hints.py")
_spec = importlib.util.spec_from_file_location("check_level_hints", _SCRIPT)
checker = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(checker)

LEVEL = """
const LevelData %(name)s = {
    plats, sizeof(plats) / sizeof(plats[0]),
    nullptr, 0,
    nullptr, 0,
    bn::regular_bg_items::w, bn::music_items::m,
    {0, 0}, {1, 1},
    700, 160%(tail)s};
"""


def level(name="LEVEL_X", tail=""):
    return LEVEL % {"name": name, "tail": tail}


def run(text, limit=28):
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "levels_worldX.h"
        path.write_text(text)
        return checker.check(path, limit), list(checker.hints_in(path))


class HintTests(unittest.TestCase):
    def test_level_without_hint_is_fine(self):
        errors, hints = run(level())
        self.assertEqual(errors, [])
        self.assertEqual(hints, [("LEVEL_X", None)])

    def test_valid_hint_is_read_and_accepted(self):
        errors, hints = run(level(tail=',\n    1,\n    "New: Dash with B"'))
        self.assertEqual(errors, [])
        self.assertEqual(hints, [("LEVEL_X", "New: Dash with B")])

    def test_hint_at_the_limit_is_accepted(self):
        text = "x" * 28
        errors, _ = run(level(tail=f',\n    1,\n    "{text}"'))
        self.assertEqual(errors, [])

    def test_hint_over_the_limit_is_an_error(self):
        text = "x" * 29
        errors, _ = run(level(tail=f',\n    1,\n    "{text}"'))
        self.assertEqual(len(errors), 1)
        self.assertIn("29 characters", errors[0])

    def test_custom_limit_is_respected(self):
        errors, _ = run(level(tail=',\n    1,\n    "Hello"'), limit=4)
        self.assertEqual(len(errors), 1)

    def test_non_ascii_hint_is_an_error(self):
        errors, _ = run(level(tail=',\n    1,\n    "Dash über Lücken"'))
        self.assertTrue(any("non-ASCII" in e for e in errors))

    def test_empty_hint_is_an_error(self):
        errors, _ = run(level(tail=',\n    1,\n    ""'))
        self.assertTrue(any("empty" in e for e in errors))

    def test_nullptr_hint_means_no_hint(self):
        errors, hints = run(level(tail=",\n    1,\n    nullptr"))
        self.assertEqual(errors, [])
        self.assertEqual(hints, [("LEVEL_X", None)])

    def test_several_levels_in_one_file(self):
        text = level("LEVEL_A", ',\n    1,\n    "ok"') + level(
            "LEVEL_B", ',\n    1,\n    "' + "y" * 40 + '"'
        )
        errors, hints = run(text)
        self.assertEqual(len(errors), 1)
        self.assertIn("LEVEL_B", errors[0])
        self.assertEqual([h[0] for h in hints], ["LEVEL_A", "LEVEL_B"])

    def test_max_chars_is_read_from_hint_logic_h(self):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "hint_logic.h").write_text("inline constexpr int MAX_CHARS = 12;")
            self.assertEqual(checker.max_chars(tmp), 12)
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(checker.max_chars(tmp), 28)


class RealLevelTests(unittest.TestCase):
    def test_real_level_hints_are_valid(self):
        here = Path(__file__).resolve().parent
        files = sorted(here.glob("levels_world*.h")) or sorted(
            here.parent.glob("**/levels_world*.h")
        )
        if not files:
            self.skipTest("levels_world*.h not found")
        limit = checker.max_chars(here if (here / "hint_logic.h").exists() else here.parent)
        errors = [e for f in files for e in checker.check(f, limit)]
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
