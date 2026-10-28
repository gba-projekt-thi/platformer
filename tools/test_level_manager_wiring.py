"""The few text checks left for level_manager.cpp.

The ORDER of load, unload, a frame and a respawn is tested on the host
(game/include/level_flow.h, tests/host/test_level_flow.cpp). What stays here
is single-call glue inside one step, where nothing can be run without Butano:
which collision probe feeds the crumbling platforms, where Restart Level
returns to, which helpers an attempt change goes through.

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
class LevelManagerGlueTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = CPP.read_text()
        cls.text = text
        cls.crumble = _method(text, "_sync_crumble_platforms")
        cls.spawn = _method(text, "_load_player_spawn", r"\s*const LevelData&\s*\w+\s*")
        cls.dynamic = _method(text, "_load_dynamic_platforms", r"\s*const LevelData&\s*\w+\s*")
        cls.apply = _method(text, "_apply_attempt", r"\s*const LevelData&\s*\w+\s*")
        cls.replay_setup = _method(text, "set_deaths_in_level", r"\s*unsigned\s+\w+\s*")

    def test_every_method_is_found(self):
        for name in ("crumble", "spawn", "dynamic", "apply", "replay_setup"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_the_duck_standing_on_a_platform_comes_from_the_bottom_probe(self):
        self.assertIn("_player.probe_bottom(Cfg::Layer::PLATFORM)", self.crumble)
        self.assertIn("_dynamic_platforms.sync_crumbling(", self.crumble)

    def test_the_level_spawn_is_remembered_for_restart(self):
        self.assertIn("_spawn_x = level.player_data.x", self.spawn)

    @unittest.skipIf(PAUSE is None, "pause_controller.cpp not found")
    def test_pause_menu_returns_the_restart_action(self):
        self.assertIn("Action::RestartRequested", PAUSE.read_text())

    def test_absent_platforms_stay_out_of_the_dynamic_helpers(self):
        self.assertIn("_dynamic_platforms.clear()", self.dynamic)
        self.assertIn("_fleeing_platforms.clear()", self.dynamic)
        self.assertIn("_present(", self.dynamic)
        self.assertIn("_fleeing_platforms.add(", self.dynamic)

    def test_apply_uses_the_host_tested_rules(self):
        self.assertIn("attempt_objects::platform_action(", self.apply)
        self.assertIn("set_present(", self.apply)

    def test_the_replay_setup_gives_no_cue(self):
        self.assertLess(
            self.replay_setup.index("_attempt.layout = deaths"),
            self.replay_setup.index("finish_respawn("),
        )


if __name__ == "__main__":
    unittest.main()
