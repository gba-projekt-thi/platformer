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


TRIGGER = _find("trigger.cpp")
MANAGER = _find("level_manager.cpp")


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

    def test_a_follower_is_pressed_through_its_state(self):
        follow = _body(self.text, r"void\s+Trigger::follow\s*\(\s*bool\s+\w+\s*\)")
        self.assertIsNotNone(follow)
        self.assertIn("_state.follow(", follow)
        self.assertIn("_refresh_visual()", follow)

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

    def test_follows_makes_a_follower_and_links_it_by_name(self):
        self.assertIn("trigger.follows != nullptr", self.triggers)
        self.assertIn("_trigger_chains.clear()", self.triggers)
        self.assertIn("_trigger_chains.link(", self.triggers)
        # The links need every trigger of the level, so they come after the
        # loop that creates them; the chain starts from their default state.
        self.assertLess(self.triggers.index("emplace_back("), self.triggers.index("_trigger_chains.link("))
        self.assertIn("_trigger_chains.capture(_triggers)", self.triggers)

    def test_every_frame_hands_the_sources_to_the_followers_after_the_collisions(self):
        update = _body(self.text, r"LevelManager::UpdateResult\s+LevelManager::update\s*\(\s*\)")
        self.assertLess(update.index("update_all()"), update.index("_trigger_chains.propagate(_triggers)"))

    def test_a_respawn_takes_the_restored_triggers_as_the_chains_start(self):
        finish = _body(self.text, r"void\s+LevelManager::_finish_respawn\s*\(\s*\)")
        self.assertLess(
            finish.index("_checkpoint_run.restore(_triggers)"),
            finish.index("_trigger_chains.capture(_triggers)"),
        )

    def test_a_level_load_forgets_the_last_levels_jump_and_dash(self):
        self.assertIn("PlayerEvents::instance().clear()", self.spawn)


if __name__ == "__main__":
    unittest.main()
