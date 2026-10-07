"""Unit tests for tools/Check_level_reachability.py.

The movement model is calibrated against the documented jump/dash envelope,
then the new dash and lever levels are checked against their design intent.
The real-level tests need game/include/level/levels_world5.h and are skipped
when it cannot be found. Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'

(about half a minute - every test simulates thousands of jumps)
"""

import importlib.util
import itertools
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_level_reachability.py")
_spec = importlib.util.spec_from_file_location("check_level_reachability", _SCRIPT)
sim = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(sim)


def _find_world5():
    here = Path(__file__).resolve().parent
    if (here / "levels_world5.h").exists():
        return here / "levels_world5.h"
    matches = sorted(here.parent.glob("**/levels_world5.h"))
    return matches[0] if matches else None


WORLD5 = _find_world5()


def two_platforms(gap):
    """Two 32x16 barrels at the same height, `gap` px apart edge to edge."""
    return [sim.Rect(0, 60, 32, 16, 0), sim.Rect(32 + gap, 60, 32, 16, 1)]


def crosses(gap, dash, optimistic):
    reached, _ = sim.reach(two_platforms(gap), (0, 30), None, dash, optimistic)
    return len(reached) == 2


class MovementModelTests(unittest.TestCase):
    """The simulator must reproduce the envelope the docs and levels rely on."""

    def test_plain_jump_crosses_the_documented_safe_gap(self):
        # Level design rule: gaps up to 40px per jump are always fine.
        self.assertTrue(crosses(40, dash=False, optimistic=False))
        self.assertTrue(crosses(56, dash=False, optimistic=False))

    def test_plain_jump_limit_is_about_61_px_conservatively(self):
        self.assertTrue(crosses(60, dash=False, optimistic=False))
        self.assertFalse(crosses(64, dash=False, optimistic=False))

    def test_run_up_and_coyote_time_add_about_11_px(self):
        self.assertTrue(crosses(70, dash=False, optimistic=True))
        self.assertFalse(crosses(76, dash=False, optimistic=True))

    def test_dash_gaps_are_out_of_reach_without_dash_even_optimistically(self):
        for gap in (76, 84, 92, 100):
            self.assertFalse(crosses(gap, dash=False, optimistic=True), gap)

    def test_jump_plus_dash_crosses_up_to_about_100_px(self):
        for gap in (76, 84, 92, 100):
            self.assertTrue(crosses(gap, dash=True, optimistic=False), gap)

    def test_nothing_crosses_a_gap_beyond_the_dash_envelope(self):
        self.assertFalse(crosses(120, dash=True, optimistic=True))
        self.assertFalse(crosses(160, dash=True, optimistic=True))

    def test_dash_does_not_add_climb(self):
        # 16px up is a normal jump; 28px up is out of reach even with dash.
        low = [sim.Rect(0, 60, 32, 16, 0), sim.Rect(60, 44, 32, 16, 1)]
        high = [sim.Rect(0, 60, 32, 16, 0), sim.Rect(60, 32, 32, 16, 1)]
        self.assertEqual(len(sim.reach(low, (0, 30), None, False, False)[0]), 2)
        self.assertEqual(len(sim.reach(high, (0, 30), None, True, True)[0]), 1)


@unittest.skipIf(WORLD5 is None, "levels_world5.h not found")
class NewLevelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.levels = sim.load_levels(WORLD5)

    def level(self, name):
        platforms, triggers, spawn, door = self.levels[name]
        sim.attach_lever_platforms(platforms, triggers)
        return platforms, triggers, spawn, door

    def test_dash_level_cannot_be_finished_without_the_dash(self):
        platforms, triggers, spawn, door = self.level("LEVEL_WORLD5_DASH")
        for optimistic in (False, True):  # even the most generous start
            _, ok, _ = sim.reach_with_levers(
                platforms, triggers, spawn, door, False, optimistic
            )
            self.assertFalse(ok, f"optimistic={optimistic}")

    def test_dash_level_can_be_finished_with_the_dash(self):
        platforms, triggers, spawn, door = self.level("LEVEL_WORLD5_DASH")
        reached, ok, _ = sim.reach_with_levers(
            platforms, triggers, spawn, door, True, False
        )
        self.assertTrue(ok)
        self.assertEqual(len(reached), len(platforms))

    def test_dash_level_gaps_sit_inside_the_dash_window(self):
        platforms, *_ = self.level("LEVEL_WORLD5_DASH")
        ordered = sorted(platforms, key=lambda p: p.left)
        gaps = [b.left - a.right for a, b in zip(ordered, ordered[1:])]
        dash_gaps = [g for g in gaps if g > 72]
        self.assertEqual(len(dash_gaps), 3)
        for g in dash_gaps:
            self.assertTrue(76 <= g <= 96, g)  # required, with timing margin
        for g in gaps:
            if g <= 72:
                self.assertLessEqual(g, 40)  # every other hop is a plain jump

    def _lever_door_reachable(self, config, dash):
        platforms, triggers, spawn, door = self.level("LEVEL_WORLD5_LEVERS")
        names = sorted(triggers)
        state = dict(zip(names, config))
        active = [
            p
            for p in platforms
            if p.trigger is None or state[p.trigger] == p.active_when_on
        ]
        return sim.reach(active, spawn, door, dash, False)[1]

    def test_lever_level_needs_lever1_whatever_the_player_does(self):
        for rest in itertools.product([False, True], repeat=3):
            self.assertFalse(self._lever_door_reachable((False, *rest), True), rest)

    def test_lever_level_plain_route_uses_levers_1_3_4_and_not_2(self):
        # lever order: lever1, lever2, lever3, lever4
        self.assertTrue(self._lever_door_reachable((True, False, True, True), False))

    def test_lever2_on_removes_the_stone_and_demands_the_dash(self):
        self.assertFalse(self._lever_door_reachable((True, True, True, True), False))
        self.assertTrue(self._lever_door_reachable((True, True, True, True), True))

    def test_one_last_room_lever_is_enough_only_with_the_dash(self):
        for config in ((True, False, True, False), (True, False, False, True)):
            self.assertFalse(self._lever_door_reachable(config, False), config)
            self.assertTrue(self._lever_door_reachable(config, True), config)

    def test_no_last_room_lever_is_impossible_even_with_the_dash(self):
        self.assertFalse(self._lever_door_reachable((True, False, False, False), True))

    def test_lever_level_is_solvable_by_flipping_levers_from_reached_platforms(self):
        platforms, triggers, spawn, door = self.level("LEVEL_WORLD5_LEVERS")
        _, ok, configs = sim.reach_with_levers(
            platforms, triggers, spawn, door, False, False
        )
        self.assertTrue(ok)
        self.assertGreater(configs, 1)

    def test_every_lever_stands_on_a_platform(self):
        _, triggers, _, _ = self.level("LEVEL_WORLD5_LEVERS")
        for name, t in triggers.items():
            self.assertIsNotNone(t["platform"], name)


if __name__ == "__main__":
    unittest.main()
