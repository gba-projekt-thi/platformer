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


if __name__ == "__main__":
    unittest.main()
