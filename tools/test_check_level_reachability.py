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
import tempfile
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


def shaft(gap=14, walls=3):
    """A floor, and on it a shaft between two columns of 26x32 barrels with
    `gap` px between them; the left column leaves a 32 px doorway at the
    bottom. Returns (platforms, spawn, door on top of the right column)."""
    plats = [sim.Rect(x, 76, 26, 32, i) for i, x in enumerate(range(-104, 120, 26))]
    left, right = 0.0, 26.0 + gap
    for k in range(walls):
        plats.append(sim.Rect(right, 44 - 32 * k, 26, 32, len(plats)))
        if k > 0:
            plats.append(sim.Rect(left, 44 - 32 * k, 26, 32, len(plats)))
    top = 44 - 32 * (walls - 1) - 16
    return plats, (-90.0, 40.0), (right, top - 4.0)


class WallJumpModelTests(unittest.TestCase):
    """Duck mirrors locomotion::Core::step() with the wall jump
    (tests/host/test_locomotion.cpp)."""

    WALL = [sim.Rect(20, 0, 26, 200, 0)]  # a wall right of the duck

    def at_the_wall(self):
        duck = sim.Duck(self.WALL, 3.0, -40.0, wall_jump=True)
        duck.on_ground = False
        for _ in range(10):
            duck.step(right=True)
        return duck

    def test_holding_toward_a_wall_slides(self):
        duck = self.at_the_wall()
        self.assertEqual(duck.wall(), 1)
        self.assertEqual(duck.vy, sim.SLIDE)

    def test_a_wall_jump_pushes_away_and_up(self):
        duck = self.at_the_wall()
        duck.step(right=True, a_held=False)
        x = duck.x
        duck.step(right=True, a_pressed=True)
        self.assertTrue(duck.wall_jumped)
        self.assertEqual(duck.vx, -sim.PUSH)
        self.assertEqual(duck.vy, sim.JUMP)
        self.assertEqual(duck.x, x - sim.PUSH)

    def test_the_duck_uses_the_games_fixed_point_values(self):
        # bn::fixed keeps 12 fraction bits: 0.38 is stored as 1556 / 4096.
        self.assertEqual(sim.fx(0.38), 1556 / 4096)
        self.assertEqual(sim.fx(-3), -3)
        duck = sim.Duck([], 0.0, 0.0)
        duck.on_ground = False
        duck.step(right=True)
        self.assertEqual(duck.vx, 1556 / 4096)

    def test_fixed_times_fixed_uses_half_precision_like_butano(self):
        # (a / 64) * (b / 64): Cfg::Player::DECELERATION * friction 1.
        self.assertEqual(sim.fx_mul(sim.fx(0.3), sim.fx(1)), 1216 / 4096)
        self.assertEqual(sim.fx_mul(-1.5, 2), -3)

    def test_without_wall_jumps_the_wall_is_ignored(self):
        duck = sim.Duck(self.WALL, 3.0, -40.0, wall_jump=False)
        for _ in range(40):
            duck.step(right=True)
        self.assertEqual(duck.wall(), 0)
        self.assertEqual(duck.vy, sim.MAXFALL)

    def test_a_shaft_needs_wall_jumps(self):
        plats, spawn, door = shaft()
        _, plain = sim.reach(plats, spawn, door, True, True)
        _, climbed = sim.reach(plats, spawn, door, False, False, allow_wall=True)
        self.assertFalse(plain)
        self.assertTrue(climbed)


class PogoModelTests(unittest.TestCase):
    """Duck's pogo strike mirrors locomotion::Core (tests/host/test_locomotion.cpp)."""

    SPIKES = [sim.Rect(0, 28, 16, 16, 0)]  # top at 20, in reach after 8 frames

    def falling_above_the_spikes(self):
        duck = sim.Duck([], 0.0, 0.0, pogo_targets=self.SPIKES)
        duck.on_ground = False
        for _ in range(8):
            duck.step()
        return duck

    def test_a_strike_over_a_trap_bounces(self):
        duck = self.falling_above_the_spikes()
        duck.step(r_pressed=True)
        self.assertTrue(duck.pogo_bounced)
        self.assertEqual(duck.vy, sim.POGO_BOUNCE)

    def test_the_bounce_is_not_cut_by_letting_go_of_a(self):
        duck = self.falling_above_the_spikes()
        duck.step(r_pressed=True)
        duck.step(a_held=False)
        self.assertEqual(duck.vy, sim.POGO_BOUNCE + sim.fx(sim.G))

    def test_no_strike_without_a_trap_below(self):
        duck = sim.Duck([], 0.0, 0.0, pogo_targets=[sim.Rect(0, 80, 16, 16, 0)])
        duck.on_ground = False
        duck.step()
        duck.step(r_pressed=True)
        self.assertFalse(duck.pogo_bounced)

    def test_constants_match_cfg_pogo_h(self):
        if REPO is None:
            self.skipTest("levels_world5.h not found")
        for sim_value, constant in (
            (sim.POGO_FRAMES, "STRIKE_FRAMES"), (sim.POGO_COOLDOWN, "COOLDOWN_FRAMES"),
            (sim.POGO_REACH, "REACH"), (sim.POGO_WIDTH, "WIDTH"), (sim.POGO_BOUNCE, "BOUNCE_SPEED"),
        ):
            self.assertEqual(sim_value, cfg_value("cfg_pogo.h", "Pogo", constant), constant)


class BossBridgeTests(unittest.TestCase):
    """A bridge bound to "<arena>_won" appears when the boss is beaten; the
    check assumes it is (the fight is the emulator replays' job)."""

    def test_the_bridge_of_a_beaten_boss_counts(self):
        plats = [sim.Rect(0, 60, 32, 16, 0), sim.Rect(150, 60, 32, 16, 1)]
        plats += [sim.Rect(x, 60, 32, 16, 2 + k, "arena_won", True)
                  for k, x in enumerate((40, 75, 110))]
        triggers = {"arena_won": {"default_on": False, "rect": (0, -200, 1, 1)}}
        sim.attach_lever_platforms(plats, triggers)
        _, ok, _ = sim.reach_with_levers(plats, triggers, (0, 40), (150, 48), False, False)
        self.assertTrue(ok)
        self.assertFalse(sim.reach(plats[:2], (0, 40), (150, 48), False, True)[1])


class PogoReachTests(unittest.TestCase):
    """A gap wider than jump + dash, with a bush to bounce off in it."""

    def gap(self):
        plats = [sim.Rect(-200, 60, 32, 16, 0), sim.Rect(-168, 60, 32, 16, 1),
                 sim.Rect(-12, 60, 32, 16, 2), sim.Rect(20, 60, 32, 16, 3)]
        bush = sim.Rect(-108, 60, 24, 24, 0)  # pit -152 .. -28, 124 px
        return plats, bush

    def test_a_gap_beyond_the_dash_needs_the_pogo(self):
        plats, bush = self.gap()
        door = (20.0, 48.0)
        self.assertFalse(sim.reach(plats, (-200, 40), door, True, True)[1])
        self.assertTrue(sim.reach(plats, (-200, 40), door, True, False, pogo_targets=[bush])[1])

    def test_static_traps_are_read_from_the_level(self):
        traps = sim.load_static_traps(WORLD5)["LEVEL_WORLD5_SCROLL"]
        self.assertIn((-240.0, 60.0, 16.0, 16.0), [(t.cx, t.cy, t.w, t.h) for t in traps])


class RunningDoorTests(unittest.TestCase):
    """A door with DoorData::flee_path has to be reached at its last stop."""

    def load(self, door):
        spec = importlib.util.spec_from_file_location(
            "generate_levels", Path(__file__).with_name("Generate_levels.py"))
        gen = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gen)
        level = {
            "name": "LEVEL_X",
            "platforms": {"name": "x_platforms", "items": [
                {"x": 0, "y": 60, "width": 32, "height": 16, "offset_x": 0,
                 "offset_y": 0, "sprite": "stone", "sprite_index": 0}]},
            "background": "world9", "music": "world9",
            "spawn": [0, 40], "door": door,
        }
        header = gen.generate({"levels": [level]}, gen.read_structs())
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "levels.h"
            path.write_text(header)
            return sim.load_levels(path)["LEVEL_X"]

    def test_a_plain_door_is_where_it_stands(self):
        _, _, _, door = self.load({"x": 30, "y": 4})
        self.assertEqual(door, (30.0, 10.0))

    def test_a_running_platform_counts_at_its_last_stop(self):
        spec = importlib.util.spec_from_file_location(
            "generate_levels", Path(__file__).with_name("Generate_levels.py"))
        gen = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gen)
        level = {
            "name": "LEVEL_X",
            "platforms": {"name": "x_platforms", "items": [
                {"x": 0, "y": 60, "width": 32, "height": 16, "offset_x": 0, "offset_y": 0,
                 "sprite": "stone", "sprite_index": 0,
                 "flee_path": [[48, -8]], "flee_range": 32, "flee_frames": 8}]},
            "background": "world9", "music": "world9", "spawn": [0, 40], "door": {"x": 30, "y": 4},
        }
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "levels.h"
            path.write_text(gen.generate({"levels": [level]}, gen.read_structs()))
            platforms = sim.load_levels(path)["LEVEL_X"][0]
        self.assertEqual((platforms[0].cx, platforms[0].cy), (48.0, 52.0))

    def test_a_running_door_counts_at_its_last_stop(self):
        _, _, _, door = self.load({"x": 30, "y": 4, "flee_path": [[40, 0], [60, -20]],
                                   "flee_range": 24, "flee_frames": 10})
        self.assertEqual(door, (90.0, -10.0))


class DeathWindowTests(unittest.TestCase):
    """Level Devil: platforms and traps only there for some attempts
    (from_death / until_death). The door must be reachable on every one."""

    def rect(self, cx, index, start=0, end=0):
        return sim.Rect(cx, 60, 32, 16, index, from_death=start, until_death=end)

    def test_attempts_split_where_a_window_opens_or_closes(self):
        self.assertEqual(sim.death_breakpoints([self.rect(0, 0)]), [0])
        self.assertEqual(
            sim.death_breakpoints([self.rect(0, 0, 0, 3), self.rect(0, 1, 3, 0), self.rect(0, 2, 2, 5)]),
            [0, 2, 3, 5])

    def test_present_follows_attempt_variant(self):
        r = self.rect(0, 0, 2, 5)
        self.assertEqual([sim.present(r, d) for d in range(7)],
                         [False, False, True, True, True, False, False])
        self.assertTrue(sim.present(self.rect(0, 0), 99))

    def level(self, bridges):
        # start 0, door platform at 160: 96 px between them, too wide for a
        # plain jump without a bridge in between
        plats = [self.rect(0, 0), self.rect(160, 1)] + bridges
        return plats, {}, (0, 40), (160.0, 48.0)

    def test_a_bridge_that_is_gone_after_3_deaths_fails_from_then_on(self):
        plats, triggers, spawn, door = self.level([self.rect(80, 2, 0, 3)])
        self.assertEqual(sim.unreachable_windows(plats, triggers, spawn, door, False, False), [3])

    def test_a_bridge_that_moves_after_3_deaths_keeps_the_level_finishable(self):
        plats, triggers, spawn, door = self.level([self.rect(80, 2, 0, 3), self.rect(72, 3, 3, 0)])
        self.assertEqual(sim.unreachable_windows(plats, triggers, spawn, door, False, False), [])

    def test_the_window_is_read_from_the_level(self):
        spec = importlib.util.spec_from_file_location(
            "generate_levels", Path(__file__).with_name("Generate_levels.py"))
        gen = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gen)
        level = {
            "name": "LEVEL_X",
            "platforms": {"name": "x_platforms", "items": [
                {"x": 0, "y": 60, "width": 32, "height": 16, "offset_x": 0, "offset_y": 0,
                 "sprite": "stone", "sprite_index": 0, "from_death": 3}]},
            "traps": {"name": "x_traps", "items": [
                {"type": "BASE", "x": 0, "y": 0, "width": 8, "height": 8, "offset_x": 0,
                 "offset_y": 0, "sprite": "b", "until_death": 2}]},
            "background": "world9", "music": "world9", "spawn": [0, 40], "door": {"x": 30, "y": 4},
        }
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "levels.h"
            path.write_text(gen.generate({"levels": [level]}, gen.read_structs()))
            platform = sim.load_levels(path)["LEVEL_X"][0][0]
            trap = sim.load_static_traps(path)["LEVEL_X"][0]
        self.assertEqual((platform.from_death, platform.until_death), (3, 0))
        self.assertEqual((trap.from_death, trap.until_death), (0, 2))


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

    def test_wall_jump_constants_match_cfg_wall_jump_h(self):
        for sim_value, constant in (
            (sim.SLIDE, "SLIDE_SPEED"), (sim.PUSH, "PUSH_SPEED"), (sim.LOCK, "LOCK_FRAMES"),
        ):
            self.assertEqual(sim_value, cfg_value("cfg_wall_jump.h", "WallJump", constant), constant)
        self.assertEqual(sim.JUMP_BUFFER, cfg_value("cfg.h", "Player", "JUMP_BUFFER_FRAMES"))

    def test_edge_margin_matches_cfg_h(self):
        self.assertEqual(sim.EDGE_MARGIN, cfg_value("cfg.h", "Screen", "EDGE_MARGIN"))

    def test_duck_body_matches_cfg_h(self):
        self.assertEqual(
            (sim.BW, sim.BH),
            (cfg_value("cfg.h", "Player", "BODY_WIDTH"), cfg_value("cfg.h", "Player", "BODY_HEIGHT")),
        )


class WallJumpUnlockTests(unittest.TestCase):
    @unittest.skipIf(REPO is None, "levels_world5.h not found")
    def test_campaign_has_the_pogo_from_world_4_and_in_every_boss_level(self):
        pogo = {level.name: level.pogo for level in sim.campaign(REPO)}
        self.assertFalse(pogo["LEVEL_3"])
        self.assertTrue(pogo["LEVEL_WORLD1_BOSS"])
        self.assertTrue(pogo["LEVEL_WORLD4_SCROLL"])

    def test_campaign_unlocks_wall_jumps_with_world_5(self):
        wall = {level.name: level.wall for level in sim.campaign(REPO)}
        self.assertFalse(wall["LEVEL_WORLD4_BOSS"])
        self.assertTrue(wall["LEVEL_WORLD5_SCROLL"])
        self.assertTrue(wall["LEVEL_WORLD5_FINALE"])


class PlayAreaTests(unittest.TestCase):
    # The duck bounces back past world_width / 2 - EDGE_MARGIN
    # (Core::set_horizontal_bound): a spawn or door out there is lost.
    def test_a_spawn_and_door_inside_the_bounce_are_fine(self):
        self.assertEqual(sim.outside_play_area((-220, 40), (220, 40), 480), [])

    def test_a_spawn_past_the_left_bounce_is_reported(self):
        self.assertEqual(sim.outside_play_area((-264, 40), (200, 40), 480), ["spawn"])

    def test_a_door_the_duck_touches_at_the_bounce_is_fine(self):
        # The duck at the bound (230) overlaps a door up to 230 + 4 + 1.
        self.assertEqual(sim.outside_play_area((0, 40), (235, 40), 480), [])

    def test_a_door_past_the_right_bounce_is_reported(self):
        self.assertEqual(sim.outside_play_area((0, 40), (236, 40), 480), ["door"])


@unittest.skipIf(REPO is None, "levels_world5.h not found")
class CampaignTests(unittest.TestCase):
    """Every level of the real game, in play order (main.cpp's levels[])."""

    @classmethod
    def setUpClass(cls):
        cls.campaign = sim.campaign(REPO)

    def test_campaign_follows_main_cpp_levels_in_play_order(self):
        names = [level.name for level in self.campaign]
        self.assertEqual(len(names), 22)
        self.assertEqual(names[0], "LEVEL_0")
        self.assertEqual(names[-1], "LEVEL_WORLD5_FINALE")

    def test_campaign_unlocks_the_dash_with_world_3(self):
        dash = {level.name: level.dash for level in self.campaign}
        self.assertFalse(dash["LEVEL_WORLD2_BOSS"])
        self.assertTrue(dash["LEVEL_3"])
        self.assertTrue(dash["LEVEL_WORLD5_DASH"])

    def test_every_spawn_and_door_lies_inside_the_play_area(self):
        self.assertEqual(sim.outside_play_areas(self.campaign), [])

    # ~20 s: the same check runs as `Check_level_reachability.py --all` in CI
    # and as a pre-commit hook whenever level data changes.
    @unittest.skipUnless(os.environ.get("SLOW_TESTS"), "set SLOW_TESTS=1 to run")
    def test_every_door_is_reachable_with_the_abilities_of_its_level(self):
        # Conservative start (no coyote time, no run-up): a normal player can
        # finish every level, so the game can be played to the end.
        self.assertEqual(sim.unreachable_doors(self.campaign), [])


if __name__ == "__main__":
    unittest.main()
