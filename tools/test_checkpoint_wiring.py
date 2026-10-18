"""Regression guard for the checkpoint wiring in LevelManager / PauseController.

The pure rules are unit-tested on the host (checkpoint_logic.h), but the glue
that calls them lives in Butano-dependent files. A method that is declared but
never called compiles and links fine and simply does nothing (that is how the
dash once lost its wiring), so these plain text checks pin the calls down.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import re
import unittest
from pathlib import Path


def _find(name):
    here = Path(__file__).resolve().parent
    if (here / name).exists():
        return here / name
    for path in sorted(here.parent.glob(f"game/**/{name}")):
        return path
    return None


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


CPP = _find("level_manager.cpp")
PAUSE = _find("pause_controller.cpp")


@unittest.skipIf(CPP is None or PAUSE is None, "level_manager.cpp / pause_controller.cpp not found")
class CheckpointWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = CPP.read_text()
        cls.update = _body(cls.text, r"LevelManager::UpdateResult\s+LevelManager::update\s*\(\s*\)")
        cls.load = _body(cls.text, r"void\s+LevelManager::load\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.unload = _body(cls.text, r"void\s+LevelManager::unload\s*\(\s*\)")
        cls.clear = _body(cls.text, r"void\s+LevelManager::_clear_runtime_state\s*\(\s*\)")
        cls.sync = _body(cls.text, r"void\s+LevelManager::_sync_checkpoints\s*\(\s*\)")

    def test_helpers_are_defined(self):
        for name in ("_load_checkpoints", "_sync_checkpoints", "_restore_checkpoint_triggers", "_clear_checkpoint"):
            self.assertRegex(self.text, rf"void\s+LevelManager::{name}\s*\(", name)

    def test_load_creates_the_checkpoints(self):
        self.assertIn("_load_checkpoints(level)", self.load)

    def test_update_syncs_checkpoints_after_the_collision_update(self):
        self.assertIn("_sync_checkpoints()", self.update)
        self.assertLess(
            self.update.index("update_all()"), self.update.index("_sync_checkpoints()")
        )

    def test_triggers_are_restored_after_the_death_reset(self):
        # The respawn (right away, or when the death freeze ends) goes through
        # _finish_respawn(): reset first, then restore the checkpoint's triggers.
        finish = _body(self.text, r"void\s+LevelManager::_finish_respawn\s*\(\s*\)")
        self.assertIsNotNone(finish)
        self.assertLess(
            finish.index("_reset_entities()"),
            finish.index("_restore_checkpoint_triggers()"),
        )
        self.assertIn("_finish_respawn()", self.update)

    def test_restart_level_clears_the_checkpoint_before_the_death(self):
        self.assertIn("RestartRequested", self.update)
        self.assertLess(self.update.index("_clear_checkpoint()"), self.update.index("_player.death()"))

    def test_pause_menu_returns_the_restart_action(self):
        self.assertIn("Action::RestartRequested", PAUSE.read_text())
        self.assertNotIn("DeathRequested", PAUSE.read_text())

    def test_load_and_unload_clear_the_checkpoint_state(self):
        for body in (self.clear, self.unload):
            self.assertIn("_checkpoints.clear()", body)
            self.assertIn("_checkpoint_tracker.reset()", body)

    def test_sync_sets_the_spawn_point_and_snapshots_the_triggers(self):
        self.assertIn("set_spawn_point", self.sync)
        self.assertIn("pack_flags", self.sync)
        self.assertIn("_checkpoint_tracker.reach(", self.sync)

    def test_the_level_spawn_is_remembered_for_restart(self):
        self.assertIn("_spawn_x = level.player_data.x", self.text)


if __name__ == "__main__":
    unittest.main()
