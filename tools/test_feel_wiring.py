"""The few text checks left for game-feel glue that has no host-testable seam.

The order of the hint, the camera, the death feedback and the respawn is tested
on the host (hint_logic.h, level_flow.h, pause_flow.h and their tests). What
stays are single lines inside one Butano step: the death ghost, and what the
pause overlay's init creates.

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


PAUSE = _find("pause_controller.cpp")
PAUSE_H = _find("pause_controller.h")
PLAYER = _find("player.cpp")
PLAYER_H = _find("player.h")


@unittest.skipIf(PLAYER is None or PLAYER_H is None, "player.cpp / player.h not found")
class PlayerDeathFeedbackWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PLAYER.read_text()
        cls.header = PLAYER_H.read_text()
        cls.begin = _body(cls.text, r"void\s+Player::begin_death_fx\s*\(\s*\)")

    def test_helpers_are_defined(self):
        self.assertIsNotNone(self.begin)

    def test_the_ghost_is_a_registered_copy_of_the_last_frame_and_the_duck_is_hidden(self):
        self.assertIn("_death_ghost.emplace(", self.begin)
        self.assertIn("set_tiles(_player_sprite.sprite().tiles())", self.begin)
        self.assertIn("_player_sprite.disable()", self.begin)



@unittest.skipIf(PAUSE is None or PAUSE_H is None, "pause_controller.cpp / pause_controller.h not found")
class PauseOverlayWiringTests(unittest.TestCase):
    """When the overlay's sprites are built and released is tested on the host
    (pause_flow.h, tests/host/test_pause_flow.cpp). What a Host test cannot
    see is whether the helpers themselves keep to it: Butano counts hidden
    sprites too (see tools/Check_sprite_budget.py)."""

    @classmethod
    def setUpClass(cls):
        cls.text = PAUSE.read_text()
        cls.header = PAUSE_H.read_text()
        cls.init = _body(cls.text, r"void\s+PauseController::_init_pause_menu\s*\(\s*\)")

    def test_nothing_is_hidden_any_more(self):
        # a hidden sprite keeps its slot: the old _set_visible(false) left the
        # last menu allocated for the rest of the level after the first pause
        self.assertNotIn("_set_visible", self.text)
        self.assertNotIn("_set_visible", self.header)

    def test_the_title_is_not_created_at_level_load(self):
        self.assertIsNotNone(self.init)
        self.assertNotIn("generate(", self.init)
        self.assertNotIn("_title_sprites", self.init)


if __name__ == "__main__":
    unittest.main()
