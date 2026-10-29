"""Unit tests for tools/tour.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("tour.py")
_spec = importlib.util.spec_from_file_location("tour", _SCRIPT)
tour = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(tour)


def picture(color, w=4, h=3):
    return bytes(b for _ in range(w * h) for b in (*color, 0))


class KeypadTests(unittest.TestCase):
    def test_two_characters_per_frame_like_butano(self):
        # low 5 key bits and the rest, each as '0' + value
        self.assertEqual(tour.keypad_commands([0, tour.KEYS["A"], tour.KEYS["R"]]), "00" "10" "08")

    def test_a_press_is_held_then_released(self):
        t = tour.Tour().press("A")
        self.assertEqual(t.frames[:3], [1, 1, 1])
        self.assertEqual(set(t.frames[3:]), {0})


class TourTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.t, cls.slot_1 = tour.build_tour(tour._levels())

    def test_shot_names_are_unique(self):
        names = [n for _, n, _ in self.t.shots]
        self.assertEqual(len(names), len(set(names)))

    def test_shots_come_in_order(self):
        frames = [f for f, _, _ in self.t.shots]
        self.assertEqual(frames, sorted(frames))
        self.assertLess(frames[-1], len(self.t.frames))

    def test_every_boss_and_story_page_is_photographed(self):
        names = {n for _, n, _ in self.t.shots}
        for world in range(1, 6):
            self.assertIn(f"boss{world}_won", names)
        for page in ("story_intro", "story_w2", "story_w3", "story_w4", "story_w5", "story_ending"):
            self.assertIn(page, names)

    def test_every_page_of_every_story_chapter_is_photographed(self):
        names = {n for _, n, _ in self.t.shots}
        counts = tour.story_page_counts()
        self.assertEqual(len(counts), 6)
        self.assertTrue(all(1 <= c <= 3 for c in counts))
        firsts = ("story_intro", "story_w2", "story_w3", "story_w4", "story_w5", "story_ending")
        for first, count in zip(firsts, counts):
            for page in range(2, count + 1):
                self.assertIn(f"{first}_p{page}", names)
            self.assertNotIn(f"{first}_p{count + 1}", names)

    def test_slot_1_starts_fresh_for_the_intro(self):
        self.assertEqual(self.slot_1[0], (0, 0))


class PictureTests(unittest.TestCase):
    def test_png_round_trip(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "a.png"
            tour.write_png(path, 4, 3, picture((10, 20, 30)))
            w, h, rgb = tour.read_png(path)
        self.assertEqual((w, h), (4, 3))
        self.assertEqual(rgb[:3], bytes((30, 20, 10)))  # stored blue, green, red

    def test_changed_share_of_pixels(self):
        a = bytes((0, 0, 0)) * 10
        b = bytes((0, 0, 0)) * 7 + bytes((200, 0, 0)) * 3
        self.assertEqual(tour.changed(a, a), 0.0)
        self.assertAlmostEqual(tour.changed(a, b), 0.3)
        # small colour noise is no change
        c = bytes((5, 5, 5)) * 10
        self.assertEqual(tour.changed(a, c), 0.0)


if __name__ == "__main__":
    unittest.main()
