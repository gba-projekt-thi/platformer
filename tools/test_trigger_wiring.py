"""Regression guard for the jump / dash trigger wiring.

The pure rules (trigger_logic.h, player_events.h) are unit-tested on the host,
but the glue lives in Butano-dependent files. A function that is declared but
never called compiles and links fine and simply does nothing, so these plain
text checks pin the calls down. Run from the repository root:

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


LOCO = _find("player_locomotion.cpp")
TRIGGER = _find("trigger.cpp")
MANAGER = _find("level_manager.cpp")


@unittest.skipIf(LOCO is None, "player_locomotion.cpp not found")
class PlayerEventWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = LOCO.read_text()
        cls.update = _body(text, r"void\s+PlayerLocomotion::update\s*\(\s*\)")
        cls.update_dash = _body(text, r"bool\s+PlayerLocomotion::update_dash\s*\(\s*\)")
        cls.jump = _body(text, r"void\s+PlayerLocomotion::handle_jump\s*\(\s*\)")

    def test_the_frame_starts_by_ending_the_previous_events(self):
        self.assertIn("PlayerEvents::instance().begin_frame()", self.update)
        self.assertLess(
            self.update.index("begin_frame()"), self.update.index("update_dash()")
        )

    def test_a_started_dash_is_noted(self):
        self.assertIn("PlayerEvents::instance().note_dash()", self.update_dash)
        self.assertLess(
            self.update_dash.index("step.started"),
            self.update_dash.index("note_dash()"),
        )

    def test_a_performed_jump_is_noted(self):
        self.assertIn("PlayerEvents::instance().note_jump()", self.jump)
        # only inside the branch that really starts the jump
        self.assertLess(
            self.jump.index("_jump_buffer_timer > 0 && _on_ground"),
            self.jump.index("note_jump()"),
        )


@unittest.skipIf(TRIGGER is None, "trigger.cpp not found")
class TriggerWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = TRIGGER.read_text()
        cls.update = _body(cls.text, r"void\s+Trigger::update\s*\(\s*\)")
        cls.enter = _body(cls.text, r"void\s+Trigger::on_enter\s*\([^)]*\)")
        cls.exit = _body(cls.text, r"void\s+Trigger::on_exit\s*\([^)]*\)")

    def test_update_ticks_the_state_and_checks_the_action(self):
        self.assertIn("_state.tick()", self.update)
        self.assertIn("action_fires(_action, _inside", self.update)
        self.assertIn("PlayerEvents::instance()", self.update)

    def test_the_overlap_callbacks_track_whether_the_duck_is_inside(self):
        self.assertIn("_inside = true", self.enter)
        self.assertIn("_inside = false", self.exit)

    def test_only_enter_triggers_fire_from_the_overlap(self):
        self.assertRegex(
            self.enter, r"if\s*\(_action\s*==\s*TriggerAction::Enter\)\s*\{\s*_state\.on_enter\(\);"
        )


@unittest.skipIf(MANAGER is None, "level_manager.cpp not found")
class LevelManagerWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = MANAGER.read_text()
        cls.triggers = _body(cls.text, r"void\s+LevelManager::_load_triggers\s*\(\s*const LevelData&\s*\w+\s*\)")
        cls.spawn = _body(cls.text, r"void\s+LevelManager::_load_player_spawn\s*\(\s*const LevelData&\s*\w+\s*\)")

    def test_delay_and_action_reach_the_trigger(self):
        self.assertIn("trigger.delay_frames", self.triggers)
        self.assertIn("trigger.action", self.triggers)

    def test_a_level_load_forgets_the_last_levels_jump_and_dash(self):
        self.assertIn("PlayerEvents::instance().clear()", self.spawn)


if __name__ == "__main__":
    unittest.main()
