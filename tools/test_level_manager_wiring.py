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
        self.assertLess(reset, self.finish.index("_checkpoint_run.restore(_triggers)"))
        self.assertIn("_finish_respawn()", self.update)

    def test_restart_level_clears_the_checkpoint_before_the_death(self):
        self.assertIn("RestartRequested", self.update)
        self.assertLess(self.update.index("_clear_checkpoint()"), self.update.index("_player.death()"))

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


if __name__ == "__main__":
    unittest.main()
