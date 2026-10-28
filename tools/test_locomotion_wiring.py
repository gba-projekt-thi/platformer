"""Regression guard for the PlayerLocomotion adapter.

The movement rules live in locomotion::Core and are tested on the host with
real inputs (tests/host/test_locomotion.cpp). What stays untestable without
Butano is the thin adapter in player_locomotion.cpp that feeds the core the
buttons and turns its effects into sounds and PlayerEvents. A dropped line
there compiles and links fine and silently breaks the dash or the jump/dash
triggers, so these plain text checks pin it down until an emulator test can
press the buttons (see the TDD plan, phase 6).

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

CPP = _ctr.find_game_file(_here.parent, "player_locomotion.cpp")
CORE = _ctr.find_game_file(_here.parent, "locomotion.h")


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


@unittest.skipIf(CPP is None or CORE is None, "player_locomotion.cpp / locomotion.h not found")
class LocomotionAdapterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.update = _body(CPP.read_text(), r"void\s+PlayerLocomotion::update\s*\(\s*\)")
        core = CORE.read_text()
        cls.input_fields = re.findall(
            r"bool\s+(\w+)\s*=", core[core.index("struct InputState"): core.index("namespace locomotion")]
        )

    def test_update_runs_the_core(self):
        self.assertIsNotNone(self.update)
        self.assertIn("_core.step(_body, input, world)", self.update)

    def test_every_button_of_input_state_is_read_from_the_keypad(self):
        self.assertTrue(self.input_fields)
        for field in self.input_fields:
            self.assertRegex(self.update, rf"input\.{field}\s*=\s*bn::keypad::{field}\(\)", field)

    def test_the_frame_starts_by_ending_the_previous_events(self):
        self.assertIn("events.begin_frame()", self.update)
        self.assertLess(self.update.index("begin_frame()"), self.update.index("_core.step("))

    def test_a_started_dash_is_noted_and_heard(self):
        branch = self.update[self.update.index("effects.dash_started"):]
        self.assertLess(branch.index("note_dash()"), branch.index("}"))
        self.assertLess(branch.index("sound_items::dash"), branch.index("}"))

    def test_a_performed_jump_is_noted_and_heard(self):
        branch = self.update[self.update.index("effects.jumped"):]
        self.assertLess(branch.index("note_jump()"), branch.index("}"))
        self.assertLess(branch.index("sound_items::jump"), branch.index("}"))

    def test_footsteps_are_heard(self):
        branch = self.update[self.update.index("effects.step_sound"):]
        self.assertLess(branch.index("sound_items::duck_step"), branch.index("}"))


if __name__ == "__main__":
    unittest.main()
