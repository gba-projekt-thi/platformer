"""Regression guard for the PlayerLocomotion wiring.

PlayerLocomotion's header declares update_dash() and the Assist Mode window
members, and the Butano-free pieces (DashController, game_modes.h) are unit
tested - but nothing tested that player_locomotion.cpp actually DEFINES and
CALLS them. A cpp that kept the pre-dash body compiled and linked fine (an
uncalled, undefined private method is no link error) and simply made the dash
and Assist Mode's wider coyote/jump-buffer windows do nothing. These checks are
plain text checks on the source, so they run anywhere Python does.

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


CPP = _find("player_locomotion.cpp")


def _function_body(text, signature_regex):
    match = re.search(signature_regex + r"\s*\{", text)
    if not match:
        return None
    start = match.end()
    depth = 1
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i]
    return None


@unittest.skipIf(CPP is None, "player_locomotion.cpp not found")
class LocomotionWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = CPP.read_text()
        cls.update = _function_body(cls.text, r"void\s+PlayerLocomotion::update\s*\(\s*\)")
        cls.update_dash = _function_body(
            cls.text, r"bool\s+PlayerLocomotion::update_dash\s*\(\s*\)"
        )
        cls.ground = _function_body(
            cls.text, r"void\s+PlayerLocomotion::update_ground_state\s*\(\s*\)"
        )

    def test_update_dash_is_defined(self):
        self.assertIsNotNone(self.update_dash, "PlayerLocomotion::update_dash() has no definition")

    def test_update_calls_update_dash(self):
        self.assertIsNotNone(self.update)
        self.assertIn("update_dash()", self.update)

    def test_update_skips_normal_movement_while_the_dash_owns_the_frame(self):
        self.assertIn("handle_horizontal_input();", self.update)
        before_input = self.update.index("update_dash()")
        self.assertLess(before_input, self.update.index("handle_horizontal_input();"))
        self.assertLess(before_input, self.update.index("apply_gravity();"))

    def test_update_dash_drives_the_controller(self):
        self.assertIn("_dash.update(", self.update_dash)
        self.assertIn("Cfg::Dash::SPEED", self.update_dash)
        self.assertIn("b_pressed()", self.update_dash)

    def test_ground_contact_refills_the_air_dash(self):
        self.assertIn("_dash.land()", self.ground)

    def test_assist_mode_windows_are_used(self):
        # set_assist_mode() only writes the members; they must be read.
        self.assertIn("_jump_buffer_frames", self.update)
        self.assertIn("_coyote_frames", self.ground)
        self.assertNotIn("Cfg::Player::JUMP_BUFFER_FRAMES", self.update)
        self.assertNotIn("Cfg::Player::COYOTE_FRAMES", self.ground)


if __name__ == "__main__":
    unittest.main()
