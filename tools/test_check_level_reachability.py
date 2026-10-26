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
import os
import re
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
    return sim._ctr.find_game_file(here.parent, "levels_world5.h")


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


class ResolveAxisTests(unittest.TestCase):
    """Mirrors resolve_axis() in the engine's move_resolution.h."""

    @staticmethod
    def floor(p):
        return p + 4 > 10  # 8 px body over a floor whose top is at 10

    def test_a_free_move_goes_all_the_way(self):
        self.assertEqual(sim.resolve_axis(0.0, 2.5, self.floor), (2.5, False))

    def test_a_fall_ends_less_than_a_pixel_above_the_floor(self):
        pos, blocked = sim.resolve_axis(3.5, 3.0, self.floor)
        self.assertTrue(blocked)
        self.assertFalse(self.floor(pos))
        self.assertGreater(pos + 4, 9)

    def test_a_small_blocked_step_does_not_move(self):
        self.assertEqual(sim.resolve_axis(5.5, 0.75, self.floor), (5.5, True))

    def test_no_move_is_never_blocked(self):
        self.assertEqual(sim.resolve_axis(6.0, 0.0, self.floor), (6.0, False))


class DashUnlockTests(unittest.TestCase):
    WORLDS = [("World 1", 0, 4), ("World 2", 4, 3), ("World 3", 7, 3)]

    def test_levels_before_the_unlock_world_have_no_dash(self):
        for index in range(7):
            self.assertFalse(sim.dash_allowed(index, self.WORLDS, 2), index)

    def test_the_unlock_world_and_everything_after_it_have_the_dash(self):
        for index in (7, 8, 9, 30):
            self.assertTrue(sim.dash_allowed(index, self.WORLDS, 2), index)


REPO = WORLD5.parents[3] if WORLD5 is not None else None


def cfg_value(header, namespace, constant):
    """`inline constexpr ... CONSTANT = value;` inside `namespace NAMESPACE { }`."""
    text = (REPO / "game/include" / header).read_text()
    block = re.search(
        r"namespace " + namespace + r" \{(.*?)\}  // namespace " + namespace, text, re.DOTALL
    ).group(1)
    return float(re.search(r"\b" + constant + r"\s*=\s*(-?[\d.]+)", block).group(1))


@unittest.skipIf(REPO is None, "levels_world5.h not found")
class ConstantsMatchTheGameTests(unittest.TestCase):
    """The simulator copies the duck's physics; a tuning change in cfg.h must
    show up here instead of silently invalidating every reachability result."""

    def test_player_constants_match_cfg_h(self):
        for sim_value, constant in (
            (sim.ACC, "ACCELERATION"),
            (sim.DECEL, "DECELERATION"),
            (sim.MAXS, "MAX_SPEED"),
            (sim.JUMP, "JUMP_SPEED"),
            (sim.G, "GRAVITY"),
            (sim.MAXFALL, "MAX_FALL_SPEED"),
            (sim.COYOTE, "COYOTE_FRAMES"),
            (sim.DEATH_Y, "DEATH_HEIGHT"),
        ):
            self.assertEqual(sim_value, cfg_value("cfg.h", "Player", constant), constant)

    def test_dash_constants_match_cfg_dash_h(self):
        self.assertEqual(sim.DASH_SPEED, cfg_value("cfg_dash.h", "Dash", "SPEED"))
        self.assertEqual(sim.DASH_FRAMES, cfg_value("cfg_dash.h", "Dash", "DURATION_FRAMES"))

    def test_duck_body_matches_cfg_h(self):
        self.assertEqual(
            (sim.BW, sim.BH),
            (cfg_value("cfg.h", "Player", "BODY_WIDTH"), cfg_value("cfg.h", "Player", "BODY_HEIGHT")),
        )


@unittest.skipIf(REPO is None, "levels_world5.h not found")
class CampaignTests(unittest.TestCase):
    """Every level of the real game, in play order (main.cpp's levels[])."""

    @classmethod
    def setUpClass(cls):
        cls.campaign = sim.campaign(REPO)

    def test_campaign_follows_main_cpp_levels_in_play_order(self):
        names = [level.name for level in self.campaign]
        self.assertEqual(len(names), 19)
        self.assertEqual(names[0], "LEVEL_0")
        self.assertEqual(names[-1], "LEVEL_WORLD5_FINALE")

    def test_campaign_unlocks_the_dash_with_world_3(self):
        dash = {level.name: level.dash for level in self.campaign}
        self.assertFalse(dash["LEVEL_WORLD2_BOSS"])
        self.assertTrue(dash["LEVEL_3"])
        self.assertTrue(dash["LEVEL_WORLD5_DASH"])

    # ~20 s: the same check runs as `Check_level_reachability.py --all` in CI
    # and as a pre-commit hook whenever level data changes.
    @unittest.skipUnless(os.environ.get("SLOW_TESTS"), "set SLOW_TESTS=1 to run")
    def test_every_door_is_reachable_with_the_abilities_of_its_level(self):
        # Conservative start (no coyote time, no run-up): a normal player can
        # finish every level, so the game can be played to the end.
        self.assertEqual(sim.unreachable_doors(self.campaign), [])


if __name__ == "__main__":
    unittest.main()
