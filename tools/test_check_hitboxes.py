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


class DoorRuleTests(unittest.TestCase):
    PICTURE = (-11, -7, 11, 16)

    def test_a_box_two_px_inside_the_picture_is_fine(self):
        self.assertEqual(checker.door_problem((-9, -5, 9, 14), self.PICTURE), None)

    def test_the_old_point_is_too_small_to_walk_into(self):
        self.assertIn("left", checker.door_problem((-1, 5, 1, 7), self.PICTURE))

    def test_a_box_past_the_picture_enters_before_the_pictures_touch(self):
        self.assertIn("right", checker.door_problem((-9, -5, 14, 14), self.PICTURE))

    def test_a_box_matching_the_picture_exactly_is_fine(self):
        self.assertEqual(checker.door_problem(self.PICTURE, self.PICTURE), None)


@unittest.skipUnless(HAS_GAME, "game/ not found")
class DoorDataTests(unittest.TestCase):
    def test_the_boxes_come_from_door_logic_h(self):
        self.assertEqual(checker.door_boxes(REPO)[32], (-9, -5, 9, 13))
        self.assertEqual(checker.door_boxes(REPO)[16], (-5, -6, 5, 6))


@unittest.skipUnless(HAS_GAME, "game/ not found")
class ScaledSpriteTests(unittest.TestCase):
    """TrapData::sprite_scale draws the picture larger around its center."""

    def test_scale_one_keeps_the_box(self):
        self.assertEqual(checker.scaled_box((-8, -6, 8, 6), 1), (-8, -6, 8, 6))

    def test_a_scaled_picture_grows_around_its_center(self):
        self.assertEqual(checker.scaled_box((-8, -6, 8, 4), 1.5), (-12, -9, 12, 6))

    def test_the_trap_scale_field_is_found_in_level_structure_h(self):
        self.assertGreater(checker.trap_field_index("sprite_scale"), 9)


class FrameRuleTests(unittest.TestCase):
    """box_problems(): every animation frame is judged on its own."""

    NO_MARGIN = (0, 0, 0, 0)

    def test_a_box_inside_every_frame_is_fine(self):
        frames = {0: (0, 0, 16, 16), 1: (1, 1, 15, 15)}
        self.assertEqual(checker.box_problems((2, 2, 14, 14), frames, self.NO_MARGIN), [])

    def test_a_frame_that_shows_less_makes_the_box_kill_on_empty_pixels(self):
        frames = {0: (0, 0, 16, 16), 1: (0, 6, 16, 16)}  # frame 1: top 6 px gone
        problems = checker.box_problems((0, 0, 16, 16), frames, self.NO_MARGIN)
        self.assertEqual(len(problems), 1)
        self.assertIn("frame 1", problems[0])
        self.assertIn("6", problems[0])

    def test_an_empty_frame_must_not_kill(self):
        problems = checker.box_problems((0, 0, 4, 4), {0: (0, 0, 4, 4), 3: None}, self.NO_MARGIN)
        self.assertEqual(len(problems), 1)
        self.assertIn("frame 3", problems[0])

    def test_the_ducks_own_picture_hides_overhang_per_frame(self):
        duck = (3, 8, 3, 0)
        self.assertEqual(checker.box_problems((-3, 0, 19, 16), {0: (0, 0, 16, 16)}, duck), [])

    def test_a_box_much_smaller_than_what_is_always_drawn_misses_the_picture(self):
        frames = {0: (0, 0, 32, 32), 1: (0, 0, 32, 32)}
        problems = checker.box_problems((8, 8, 24, 24), frames, self.NO_MARGIN)
        self.assertEqual(len(problems), 4)  # every side is 8 px in
        self.assertIn("left", problems[0])

    def test_parts_drawn_in_only_some_frames_do_not_count_as_always_drawn(self):
        # Frame 1 reaches 10 px further right; only frame 0's part is always
        # there.
        frames = {0: (0, 0, 16, 16), 1: (0, 0, 26, 16)}
        self.assertEqual(checker.box_problems((0, 0, 16, 16), frames, self.NO_MARGIN), [])

    def test_inset_within_the_under_tolerance_is_fine(self):
        frames = {0: (0, 0, 16, 16)}
        inset = checker.TRAP_UNDER_TOLERANCE
        self.assertEqual(
            checker.box_problems((inset, 0, 16 - inset, 16), frames, self.NO_MARGIN), [])

    def test_numbers_report_each_frame(self):
        numbers = checker.frame_numbers(
            (0, 0, 16, 16), {0: (0, 0, 16, 16), 1: (0, 4, 16, 16)}, self.NO_MARGIN)
        self.assertEqual(numbers[0], 0)
        self.assertEqual(numbers[1], 4)


class TrapFramesTests(unittest.TestCase):
    SEQUENCES = {"SEQ": [0, 1, 2, 3, 4], "ONE": [2]}

    def test_no_animation_shows_frame_zero(self):
        self.assertEqual(checker.trap_frames("TRAP_GRAPHICS_INDEXES_0", self.SEQUENCES, 3), [0])

    def test_frames_wrap_like_the_game_and_are_listed_once(self):
        self.assertEqual(checker.trap_frames("SEQ", self.SEQUENCES, 3), [0, 1, 2])

    def test_unknown_sequence_is_an_error(self):
        with self.assertRaises(ValueError):
            checker.trap_frames("NOPE", self.SEQUENCES, 3)


class DuckFrameTests(unittest.TestCase):
    # Body 8x8, picture drawn 4 px up (feet on the body's bottom).
    def test_a_picture_that_matches_the_body_is_fine(self):
        frames = {0: (-5, -8, 5, 8)}
        self.assertEqual(checker.duck_problems(frames, 8, 8, -4), [])

    def test_body_reaching_past_the_picture_is_unfair(self):
        frames = {0: (-2, -8, 2, 8)}  # a 4 px wide picture, 8 px body
        problems = checker.duck_problems(frames, 8, 8, -4)
        self.assertTrue(any("frame 0" in p and "body" in p for p in problems))

    def test_feet_floating_above_the_body_are_found(self):
        frames = {0: (-5, -8, 5, 4)}  # feet 4 px above the body's bottom
        problems = checker.duck_problems(frames, 8, 8, -4)
        self.assertTrue(any("feet" in p for p in problems))

    def test_picture_much_wider_than_the_body_is_found(self):
        frames = {0: (-12, -8, 12, 8)}
        problems = checker.duck_problems(frames, 8, 8, -4)
        self.assertTrue(any("sideways" in p for p in problems))

    def test_mirrored_pictures_are_judged_both_ways(self):
        # Wall slide: drawn for a wall on the right, flipped for the left.
        frames = {0: (-6, -8, 8, 8)}
        self.assertEqual(checker.duck_problems(frames, 8, 8, -4), [])


class CoverageTests(unittest.TestCase):
    def test_a_sprite_with_a_body_that_nothing_checks_fails(self):
        problems = checker.coverage_problems({"a", "b"}, {"a"}, {})
        self.assertEqual(len(problems), 1)
        self.assertIn("b", problems[0])

    def test_a_sprite_without_a_body_needs_a_reason(self):
        self.assertEqual(checker.coverage_problems({"a", "b"}, {"a"}, {"b": "title screen"}), [])

    def test_a_no_body_sprite_that_is_checked_anyway_is_stale(self):
        problems = checker.coverage_problems({"a"}, {"a"}, {"a": "title screen"})
        self.assertEqual(len(problems), 1)
        self.assertIn("a", problems[0])

    def test_a_no_body_entry_for_a_missing_sprite_is_stale(self):
        problems = checker.coverage_problems({"a"}, {"a"}, {"gone": "old"})
        self.assertEqual(len(problems), 1)


@unittest.skipUnless(HAS_GAME, "game/ not found")
class AllSpritesDataTests(unittest.TestCase):
    def test_every_sprite_sheet_is_checked_or_has_no_body(self):
        covered = set()
        checker.check(REPO, covered=covered)
        sheets = checker.sprite_sheet_names(REPO)
        self.assertEqual(checker.coverage_problems(sheets, covered, checker.NO_BODY), [])

    def test_bodies_of_the_code_and_the_levels_are_all_checked(self):
        covered = set()
        checker.check(REPO, covered=covered)
        for name in ("ente", "pogo16x16", "checkpoint16x16", "lever16x16", "door32x32",
                     "susanne16x16", "thwomp64x64", "mimic32x32"):
            self.assertIn(name, covered)

    def test_the_cfg_constants_are_read_from_the_code(self):
        self.assertEqual(checker.pogo_box(REPO), (-8, -6, 8, 6))
        self.assertEqual(checker.checkpoint_zone(REPO)[0], 16)


class RealDataTests(unittest.TestCase):
    def test_duck_feet_rest_on_the_bottom_of_its_body(self):
        _, _, _, below_feet = checker.duck_margins(REPO)
        self.assertEqual(below_feet, 0)

    def test_every_platform_and_trap_matches_its_sprite(self):
        self.assertEqual(checker.check(REPO), [])


if __name__ == "__main__":
    unittest.main()
