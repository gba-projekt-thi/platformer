"""Regression guard for the call order in LevelManager.

The rules behind the dynamic platforms (DynamicPlatforms) and the checkpoints
(CheckpointRun) are tested on the host with stand-ins
(tests/host/test_dynamic_platforms.cpp, test_checkpoint_run.cpp). What is left
in the Butano-dependent level_manager.cpp is when they are called. A dropped
call compiles and links fine and silently does nothing, so these plain text
checks pin the order down until an emulator test can play a level (see the TDD
plan, phase 6).

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import re
import unittest
from pathlib import Path

_here = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location(
    "check_trigger_references", _here / "Check_trigger_references.py"
)
_ctr = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_ctr)

CPP = _ctr.find_game_file(_here.parent, "level_manager.cpp")
PAUSE = _ctr.find_game_file(_here.parent, "pause_controller.cpp")


def _body(text, signature):
    match = re.search(signature + r"\s*\{", text)
    if not match:
        return None
    depth, start = 1, match.end()
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i]
    return None


def _method(text, name, args=r""):
    return _body(text, r"LevelManager::" + name + r"\s*\(" + args + r"\)")


@unittest.skipIf(CPP is None, "level_manager.cpp not found")
class LevelManagerWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = CPP.read_text()
        cls.text = text
        cls.load = _method(text, "load", r"\s*const LevelData&\s*\w+\s*")
        cls.update = _method(text, "update")
        cls.unload = _method(text, "unload")
        cls.clear = _method(text, "_clear_runtime_state")
        cls.finish = _method(text, "_finish_respawn")
        cls.crumble = _method(text, "_sync_crumble_platforms")

    def test_every_method_is_found(self):
        for name in ("load", "update", "unload", "clear", "finish", "crumble"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_load_creates_the_dynamic_platforms_after_platforms_and_triggers(self):
        dynamic = self.load.index("_load_dynamic_platforms(level)")
        self.assertLess(self.load.index("_load_platforms(level)"), dynamic)
        self.assertLess(self.load.index("_load_triggers(level)"), dynamic)
        self.assertLess(dynamic, self.load.index("_dynamic_platforms.sync_switched("))

    def test_load_creates_the_checkpoints(self):
        self.assertIn("_load_checkpoints(level)", self.load)

    def test_every_frame_syncs_platforms_and_checkpoints_after_the_collision_update(self):
        collisions = self.update.index("update_all()")
        for call in ("_sync_checkpoints()", "_dynamic_platforms.sync_switched(", "_sync_crumble_platforms()"):
            self.assertLess(collisions, self.update.index(call), call)

    def test_the_duck_standing_on_a_platform_comes_from_the_bottom_probe(self):
        self.assertIn("_player.probe_bottom(Cfg::Layer::PLATFORM)", self.crumble)
        self.assertIn("_dynamic_platforms.sync_crumbling(", self.crumble)

    def test_a_respawn_resets_entities_then_platforms_and_checkpoint_triggers(self):
        reset = self.finish.index("_reset_entities()")
        self.assertLess(reset, self.finish.index("_dynamic_platforms.reset_crumbling()"))
        self.assertLess(reset, self.finish.index("_trigger_chains.respawn("))
        self.assertIn("_checkpoint_run.restore(", self.finish)
        # through _respawn() (respawn_logic.h decides when)
        self.assertIn("_respawn(respawn::Cause::Death)", self.update)

    def test_restart_level_clears_the_checkpoint_before_the_death(self):
        self.assertIn("RestartRequested", self.update)
        self.assertLess(self.update.index("_clear_checkpoint()"), self.update.index("_player.death()"))

    @unittest.skipIf(PAUSE is None, "pause_controller.cpp not found")
    def test_restart_level_sets_up_the_next_attempt_before_the_world_moves(self):
        # The duck is at the spawn at once: the next attempt's layout must be
        # there before the physics step, or an old trap kills it twice.
        restart = self.update.index("RestartRequested")
        death = self.update.index("_player.death()", restart)
        respawn = self.update.index("_respawn(respawn::Cause::Restart)", restart)
        self.assertLess(death, respawn)
        self.assertLess(respawn, self.update.index("update_all()"))
        respawn_body = _method(self.text, "_respawn", r"\s*respawn::Cause\s+\w+\s*") or ""
        self.assertIn("_pending_respawn.request(cause, _death_fx.frozen())", respawn_body)
        self.assertIn("_finish_respawn()", respawn_body)

    def test_restart_level_is_exactly_one_death(self):
        # Its death is taken (and saved) right there, so the end of the frame
        # does not set up another attempt for it.
        restart = self.update[self.update.index("RestartRequested"):self.update.index("ReturnToTitleRequested")]
        self.assertEqual(restart.count("_player.death()"), 1)
        death = restart.index("_player.death()")
        self.assertLess(death, restart.index("_deaths_seen = _player.get_deaths()"))
        self.assertLess(restart.index("_deaths_seen = _player.get_deaths()"), restart.index("_save_sync.sync()"))
        self.assertLess(restart.index("_save_sync.sync()"), restart.index("_respawn(respawn::Cause::Restart)"))
        end = self.update[self.update.index("update_all()"):]
        self.assertIn("_respawn(respawn::Cause::Death)", end)

    @unittest.skipIf(PAUSE is None, "pause_controller.cpp not found")
    def test_pause_menu_returns_the_restart_action(self):
        self.assertIn("Action::RestartRequested", PAUSE.read_text())

    def test_load_and_unload_forget_platforms_and_checkpoints(self):
        self.assertIn("_clear_runtime_state()", self.load)
        self.assertIn("_clear_runtime_state()", self.unload)
        for call in ("_dynamic_platforms.clear()", "_checkpoints.clear()", "_checkpoint_run = CheckpointRun()"):
            self.assertIn(call, self.clear, call)
        # The non-owning views go before the containers they point into.
        self.assertLess(self.clear.index("_dynamic_platforms.clear()"), self.clear.index("_platforms.clear()"))

    def test_the_level_spawn_is_remembered_for_restart(self):
        self.assertIn("_spawn_x = level.player_data.x", self.text)

    # -- false exits (door_logic.h) ------------------------------------------

    def test_the_deaths_of_this_level_count_from_its_entry(self):
        seen = self.text.index("_deaths_seen = _player.get_deaths();")
        entry = self.text.index("_deaths_at_entry = attempt_variant::entry_deaths(")
        # set right after _deaths_seen on level load
        self.assertLess(seen, entry)
        self.assertLess(entry - seen, 400)
        deaths = _body(self.text, r"unsigned LevelManager::_deaths_in_level\(\) const") or ""
        # whitespace aside (clang-format wraps the call)
        self.assertIn(
            "attempt_variant::deaths_in_level(_player.get_deaths(),_deaths_at_entry)",
            re.sub(r"\s+", "", deaths),
        )

    def test_the_entry_is_kept_in_the_save_for_continue(self):
        # Title -> Continue must not set the level back to its first attempt.
        body = re.sub(r"\s+", "", self.text[self.text.index("_deaths_at_entry = attempt_variant"):])
        self.assertTrue(body.startswith(
            "_deaths_at_entry=attempt_variant::entry_deaths(state.entry_level,state.entry_deaths,"
            "state.level,_player.get_deaths());"))
        self.assertIn("state.entry_level=state.level;", body[:400])
        self.assertIn("state.entry_deaths=uint16_t(_deaths_at_entry);", body[:400])
        # a continued level starts in the layout of its deaths: no cue for it
        self.assertIn("_layout_deaths=_deaths_in_level();", body[:400])

    def test_the_door_is_placed_on_load_and_after_every_respawn(self):
        place = _method(self.text, "_place_door") or ""
        self.assertIn("place_for_attempt(_deaths_in_level())", place)
        self.assertLess(self.load.index("_load_door(level)"), self.load.index("_place_door()"))
        # a death, a checkpoint respawn and Restart Level all end here
        self.assertIn("_place_door()", self.finish)

    def test_the_door_trap_trigger_is_found_after_the_triggers_are_loaded(self):
        self.assertLess(
            self.load.index("_load_triggers(level)"),
            self.load.index("_door_trap = find_trigger(level.door.trap_trigger_name)"),
        )
        self.assertIn("_door_trap = nullptr", self.clear)

    def test_a_trapped_door_kills_before_the_death_is_noticed(self):
        # Like a trap: the death of this frame starts the death feedback.
        check = self.update.index("_check_door_trap()")
        self.assertLess(self.update.index("update_all()"), check)
        self.assertLess(check, self.update.index("_check_player_died()"))
        trap = _method(self.text, "_check_door_trap") or ""
        self.assertIn("door_logic::on_touch(", trap)
        self.assertIn("_player.death()", trap)

    def test_a_duck_a_trap_caught_on_the_trapped_door_dies_once(self):
        trap = _method(self.text, "_check_door_trap") or ""
        self.assertIn("_player.get_deaths() != _deaths_seen", trap)

    def test_a_checkpoint_never_keeps_the_door_trap_trigger_on(self):
        # Restored after a death, it would make the door deadly for good.
        found = self.load.index("_door_trap = find_trigger(level.door.trap_trigger_name)")
        self.assertLess(found, self.load.index("_checkpoint_run.forget_trigger("))


@unittest.skipIf(CPP is None, "level_manager.cpp not found")
class AttemptObjectsWiringTests(unittest.TestCase):
    """Level Devil: traps and platforms with a death window (from_death /
    until_death). The rules are host-tested (test_attempt_variant.cpp,
    test_attempt_objects.cpp); these pin down when LevelManager applies them."""

    @classmethod
    def setUpClass(cls):
        text = CPP.read_text()
        cls.text = text
        cls.load = _method(text, "load", r"\s*const LevelData&\s*\w+\s*")
        cls.spawn = _method(text, "_load_player_spawn", r"\s*const LevelData&\s*\w+\s*")
        cls.finish = _method(text, "_finish_respawn")
        cls.apply = _method(text, "_apply_attempt", r"\s*const LevelData&\s*\w+\s*")
        cls.dynamic = _method(text, "_load_dynamic_platforms", r"\s*const LevelData&\s*\w+\s*")
        cls.deaths = _body(text, r"LevelManager::_deaths_in_level\s*\(\)\s*const")

    def test_every_method_is_found(self):
        for name in ("load", "spawn", "finish", "apply", "dynamic", "deaths"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_the_run_total_is_taken_where_the_death_feedback_takes_it(self):
        seen = self.spawn.index("_deaths_seen = _player.get_deaths()")
        self.assertLess(seen, self.spawn.index("_deaths_at_entry = attempt_variant::entry_deaths("))

    def test_deaths_in_level_count_from_the_entry(self):
        self.assertIn(
            "attempt_variant::deaths_in_level(_player.get_deaths(),_deaths_at_entry)",
            re.sub(r"\s+", "", self.deaths))

    def test_load_applies_the_windows_once_everything_exists(self):
        apply = self.load.index("_apply_attempt(level)")
        for call in ("_load_platforms(level)", "_load_dynamic_platforms(level)", "_load_traps(level)"):
            self.assertLess(self.load.index(call), apply, call)
        self.assertLess(apply, self.load.index("_dynamic_platforms.sync_switched("))

    def test_a_respawn_applies_them_after_the_reset_and_before_the_platform_resets(self):
        # Covers a death, the checkpoint respawn and Restart Level: all of them
        # end in _finish_respawn().
        apply = self.finish.index("_apply_attempt(")
        dynamic = self.finish.index("_load_dynamic_platforms(")
        self.assertLess(self.finish.index("_reset_entities()"), apply)
        self.assertLess(apply, dynamic)
        self.assertLess(dynamic, self.finish.index("_dynamic_platforms.reset_crumbling()"))
        self.assertLess(dynamic, self.finish.index("_fleeing_platforms.reset()"))

    def test_absent_platforms_stay_out_of_the_dynamic_helpers(self):
        self.assertIn("_dynamic_platforms.clear()", self.dynamic)
        self.assertIn("_fleeing_platforms.clear()", self.dynamic)
        self.assertIn("_present(", self.dynamic)
        self.assertIn("_fleeing_platforms.add(", self.dynamic)

    def test_a_respawn_into_a_changed_layout_gives_the_cue(self):
        # Compared with the layout that was there (not deaths - 1: Restart
        # Level during a freeze skips one), once the new one is applied.
        changed = self.finish.index("attempt_objects::layout_changes(")
        self.assertLess(changed, self.finish.index("_apply_attempt("))
        self.assertIn("_layout_deaths", self.finish)
        cue = _method(self.text, "_cue_layout_change") or ""
        self.assertIn("_death_fx.shake(Cfg::DeathFx::LAYOUT_SHAKE_FRAMES)", cue)
        self.assertIn("bn::sound_items::layout_shift", cue)
        self.assertLess(self.finish.index("_apply_attempt("), self.finish.index("_cue_layout_change()"))

    def test_level_load_and_the_replay_setup_give_no_cue(self):
        # the layout the load applies (attempt 0, or a continued level's own)
        self.assertIn("_layout_deaths = _deaths_in_level()", self.spawn)
        setup = _method(self.text, "set_deaths_in_level", r"\s*unsigned\s+\w+\s*") or ""
        self.assertLess(setup.index("_layout_deaths = deaths"), setup.index("_finish_respawn()"))
        self.assertNotIn("_cue_layout_change()", self.load)

    def test_apply_uses_the_host_tested_rules(self):
        self.assertIn("attempt_objects::platform_action(", self.apply)
        self.assertIn("set_present(", self.apply)


if __name__ == "__main__":
    unittest.main()
