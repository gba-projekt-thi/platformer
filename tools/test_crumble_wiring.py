"""Regression guard for the crumbling-platform wiring in LevelManager.

crumble_logic.h is unit-tested on the host, but the glue that applies its
events lives in a Butano-dependent file. A function that is never called
compiles and links fine and simply does nothing, so these plain text checks pin
the calls down. Run from the repository root:

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


MANAGER = _find("level_manager.cpp")


@unittest.skipIf(MANAGER is None, "level_manager.cpp not found")
class CrumbleWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = MANAGER.read_text()
        cls.update = _body(cls.text, r"LevelManager::UpdateResult\s+LevelManager::update\s*\(\s*\)")
        cls.load = _body(cls.text, r"void\s+LevelManager::load\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.unload = _body(cls.text, r"void\s+LevelManager::unload\s*\(\s*\)")
        cls.clear = _body(cls.text, r"void\s+LevelManager::_clear_runtime_state\s*\(\s*\)")
        cls.finish = _body(cls.text, r"void\s+LevelManager::_finish_respawn\s*\(\s*\)")
        cls.create = _body(cls.text, r"void\s+LevelManager::_load_crumble_platforms\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.sync = _body(cls.text, r"void\s+LevelManager::_sync_crumble_platforms\s*\(\s*\)")
        cls.reset = _body(cls.text, r"void\s+LevelManager::_reset_crumble_platforms\s*\(\s*\)")

    def test_helpers_are_defined(self):
        for name in ("create", "sync", "reset", "finish", "update", "load"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_the_platforms_are_created_after_the_platform_bodies(self):
        self.assertLess(self.load.index("_load_platforms(level)"), self.load.index("_load_crumble_platforms(level)"))

    def test_a_crumbling_platform_cannot_also_be_switched(self):
        self.assertIn("platform.trigger_name == nullptr", self.create)
        self.assertIn("platform.crumble_frames", self.create)
        self.assertIn("platform.crumble_respawn_frames", self.create)

    def test_every_frame_steps_the_platforms_after_the_collision_update(self):
        self.assertIn("_sync_crumble_platforms()", self.update)
        self.assertLess(self.update.index("update_all()"), self.update.index("_sync_crumble_platforms()"))

    def test_the_duck_standing_on_a_platform_comes_from_the_bottom_probe(self):
        self.assertIn("_player.probe_bottom(Cfg::Layer::PLATFORM)", self.sync)
        self.assertIn("below.hits[hit].body == crumbling.body", self.sync)

    def test_the_events_are_applied_to_body_and_sprite(self):
        self.assertIn("state.update(standing, blocked)", self.sync)
        for event, call in (
            ("Fell", "body->disable()"),
            ("Vanished", "sprite->disable()"),
            ("Respawned", "body->enable()"),
        ):
            branch = self.sync[self.sync.index(f"crumble::Event::{event}"):]
            self.assertIn(call, branch.split("break;")[0], event)
        self.assertIn("sprite->enable()", self.sync)

    def test_a_platform_does_not_come_back_inside_the_duck(self):
        self.assertIn("switch_logic::boxes_overlap(", self.sync)

    def test_the_sprite_shakes_and_falls_through_its_offsets(self):
        self.assertIn("pos.offset_x = crumbling.state.shake_offset()", self.sync)
        self.assertIn("pos.offset_y = crumbling.state.fall_offset()", self.sync)

    def test_a_respawn_puts_every_platform_back(self):
        self.assertIn("_reset_crumble_platforms()", self.finish)
        for call in ("state.reset()", "body->enable()", "sprite->enable()"):
            self.assertIn(call, self.reset)

    def test_load_and_unload_forget_the_platforms(self):
        for body in (self.clear, self.unload):
            self.assertIn("_crumble_platforms.clear()", body)


if __name__ == "__main__":
    unittest.main()
