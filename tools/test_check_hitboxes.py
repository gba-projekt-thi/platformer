"""Unit tests for tools/Check_hitboxes.py.

The rules are tested on small hand-made sprites; the real-data tests need the
repository's game/ folder and are skipped when it cannot be found. Run from
the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_hitboxes.py")
_spec = importlib.util.spec_from_file_location("check_hitboxes", _SCRIPT)
checker = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(checker)

REPO = Path(__file__).resolve().parent.parent
HAS_GAME = (REPO / "game/include/level/levels_world1.h").exists()


def picture(*lines):
    """Rows of palette indexes from strings: '.' = transparent, digit = index."""
    return [[0 if c == "." else int(c) for c in line] for line in lines]


class BmpTests(unittest.TestCase):
    def roundtrip(self, rows, bpp):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "x.bmp"
            checker.write_bmp(path, rows, bpp)
            return checker.read_bmp(path)

    def test_8bpp_roundtrip(self):
        rows = picture("12......", "........", "......34")
        self.assertEqual(self.roundtrip(rows, 8), rows)

    def test_4bpp_roundtrip_with_odd_width(self):
        rows = picture("1.......5", ".........", "7.......2")
        self.assertEqual(self.roundtrip(rows, 4), rows)


class FrameBoxTests(unittest.TestCase):
    # Two 4x4 frames stacked vertically, like a Butano sprite sheet.
    SHEET = picture(
        "....",
        ".11.",
        ".11.",
        "....",
        "2...",
        "2...",
        "2...",
        "2...",
    )

    def test_box_is_relative_to_the_frame_center(self):
        self.assertEqual(checker.frame_box(self.SHEET, 4, 0), (-1, -1, 1, 1))

    def test_frame_index_selects_the_frame(self):
        self.assertEqual(checker.frame_box(self.SHEET, 4, 1), (-2, -2, -1, 2))

    def test_frame_index_wraps_like_the_game(self):
        # LevelManager uses sprite_index % graphics_count.
        self.assertEqual(checker.frame_box(self.SHEET, 4, 3), (-2, -2, -1, 2))

    def test_empty_frame_has_no_box(self):
        self.assertIsNone(checker.frame_box(picture("..", ".."), 2, 0))


class PlatformRuleTests(unittest.TestCase):
    def test_matching_platform_has_no_mismatch(self):
        self.assertEqual(checker.platform_mismatch((0, 10, 32, 26), (0, 10, 32, 26)), (0, 0, 0))

    def test_hitbox_below_the_picture_lets_the_duck_sink_in(self):
        sink, _, _ = checker.platform_mismatch((0, 21, 32, 31), (0, 10, 32, 42))
        self.assertEqual(sink, 11)

    def test_hitbox_above_the_picture_lets_the_duck_float(self):
        sink, _, _ = checker.platform_mismatch((0, 8, 32, 24), (0, 10, 32, 26))
        self.assertEqual(sink, -2)

    def test_wider_hitbox_carries_the_duck_in_the_air(self):
        _, left, right = checker.platform_mismatch((-8, 0, 24, 16), (0, 0, 16, 16))
        self.assertEqual((left, right), (8, 8))

    def test_narrower_hitbox_drops_the_duck_off_a_visible_edge(self):
        _, left, right = checker.platform_mismatch((9, 0, 41, 16), (0, 0, 50, 16))
        self.assertEqual((left, right), (-9, -9))


class TrapRuleTests(unittest.TestCase):
    # The duck's picture reaches past its body: 3 px sideways, 8 px above its
    # head, 0 px below its feet.
    DUCK = (3, 8, 3, 0)

    def test_hitbox_inside_the_picture_is_fair(self):
        self.assertEqual(checker.trap_unfair_by((2, 2, 14, 14), (0, 0, 16, 16), self.DUCK), 0)

    def test_hitbox_above_the_picture_kills_before_the_feet_touch_it(self):
        self.assertEqual(checker.trap_unfair_by((0, -4, 16, 16), (0, 0, 16, 16), self.DUCK), 4)

    def test_overhang_hidden_by_the_ducks_own_picture_is_fair(self):
        # 3 px to the side and 8 px below the trap are covered by the duck.
        self.assertEqual(checker.trap_unfair_by((-3, 0, 19, 24), (0, 0, 16, 16), self.DUCK), 0)
        self.assertEqual(checker.trap_unfair_by((-5, 0, 16, 16), (0, 0, 16, 16), self.DUCK), 2)


@unittest.skipUnless(HAS_GAME, "game/ not found")
class RealDataTests(unittest.TestCase):
    def test_duck_feet_rest_on_the_bottom_of_its_body(self):
        _, _, _, below_feet = checker.duck_margins(REPO)
        self.assertEqual(below_feet, 0)

    def test_every_platform_and_trap_matches_its_sprite(self):
        self.assertEqual(checker.check(REPO), [])


if __name__ == "__main__":
    unittest.main()
