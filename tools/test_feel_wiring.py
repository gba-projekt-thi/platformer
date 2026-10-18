"""Regression guard for the game-feel wiring (hint, camera, death feedback).

The pure rules (hint_logic.h, camera_rig.h, death_fx.h) are unit-tested on the
host, but the glue that calls them lives in Butano-dependent files. A function
that is declared but never called compiles and links fine and simply does
nothing (that is how the dash once lost its wiring), so these plain text checks
pin the calls down. They run anywhere Python does.

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


SCENE = _find("level_scene.cpp")
MANAGER_H = _find("level_manager.h")
MANAGER = _find("level_manager.cpp")


@unittest.skipIf(SCENE is None or MANAGER_H is None, "level_scene.cpp / level_manager.h not found")
class HintWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = SCENE.read_text()
        cls.update = _body(cls.text, r"void\s+LevelScene::update\s*\(\s*\)")
        cls.update_hint = _body(cls.text, r"void\s+LevelScene::_update_hint\s*\(\s*\)")
        cls.show = _body(cls.text, r"void\s+LevelScene::_show_hint_if_needed\s*\(\s*\)")
        cls.draw = _body(cls.text, r"void\s+LevelScene::_draw_hint\s*\(\s*\)")
        cls.manager_h = MANAGER_H.read_text()

    def test_helpers_are_defined(self):
        for name in ("update_hint", "show", "draw", "update"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_level_manager_exposes_the_pause_state(self):
        self.assertRegex(self.manager_h, r"auto\s+paused\s*\(\s*\)\s*const\s*->\s*bool")
        self.assertIn("_pause_controller.paused()", self.manager_h)

    def test_the_hint_follows_the_pause_state(self):
        self.assertIn("_level_manager.paused()", self.update_hint)
        self.assertIn("hint::should_draw(", self.update_hint)
        self.assertIn("_draw_hint()", self.update_hint)

    def test_the_countdown_and_b_only_run_while_the_game_does(self):
        gate = self.update_hint.index("if (!paused)")
        self.assertLess(gate, self.update_hint.index("b_pressed()"))
        self.assertLess(gate, self.update_hint.index("_hint_timer.tick()"))

    def test_the_hint_sprites_are_freed_before_the_pause_overlay_allocates(self):
        # LevelManager::update() opens the pause menu, which allocates ~57
        # sprites in that very call, so the hint has to go first.
        free = self.update.index("_hint_sprites.clear()")
        self.assertIn("start_pressed()", self.update[:free])
        self.assertLess(free, self.update.index("_level_manager.update()"))

    def test_the_hint_text_is_kept_to_draw_it_again_after_a_pause(self):
        self.assertIn("_hint_text = text", self.show)
        self.assertIn("_hint_text", self.draw)
        self.assertIn("_draw_hint()", self.show)


@unittest.skipIf(MANAGER is None or MANAGER_H is None, "level_manager.cpp / level_manager.h not found")
class CameraWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = MANAGER.read_text()
        cls.header = MANAGER_H.read_text()
        cls.update = _body(cls.text, r"LevelManager::UpdateResult\s+LevelManager::update\s*\(\s*\)")
        cls.setup = _body(cls.text, r"void\s+LevelManager::_setup_camera\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.step = _body(cls.text, r"void\s+LevelManager::_update_camera\s*\(\s*\)")

    def test_helpers_are_defined(self):
        self.assertIsNotNone(self.setup)
        self.assertIsNotNone(self.step)
        self.assertIn("CameraRig<bn::fixed> _camera_rig", self.header)

    def test_update_steps_the_rig_instead_of_gluing_the_camera_to_the_duck(self):
        self.assertIn("_update_camera()", self.update)
        self.assertNotIn("follow(_player.pos.x", self.update)
        self.assertLess(self.update.index("_update_camera()"), self.update.index("sync_all("))

    def test_the_step_feeds_the_rig_and_follows_its_result(self):
        self.assertIn("_camera_rig.update(", self.step)
        self.assertIn("_player.vel_x", self.step)
        self.assertIn("up_held()", self.step)
        self.assertIn("down_held()", self.step)
        self.assertIn("follow(_camera_rig.x(), _camera_rig.y())", self.step)

    def test_level_load_sets_the_bounds_and_cuts_to_the_duck(self):
        self.assertIn("_camera_rig.set_bounds(", self.setup)
        self.assertIn("camera_rig::max_offset(", self.setup)
        self.assertLess(self.setup.index("set_bounds("), self.setup.index("snap("))
        self.assertLess(self.setup.index("snap("), self.setup.index("follow(_camera_rig.x()"))

    def test_a_respawn_cuts_instead_of_gliding(self):
        death = self.update[self.update.index("_save_sync.sync()"):]
        self.assertIn("_camera_rig.snap(", death)
        self.assertLess(death.index("_reset_entities()"), death.index("_camera_rig.snap("))


if __name__ == "__main__":
    unittest.main()
