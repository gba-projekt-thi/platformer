"""The two text checks left for level_manager.cpp.

The ORDER of load, unload, a frame and a respawn is tested on the host
(game/include/level_flow.h, tests/host/test_level_flow.cpp), and so are what an
attempt does to platforms and traps (attempt_objects.h) and the pause menu's
outcomes (pause_flow.h). What stays here is a single call inside one step,
where nothing can be run without Butano: which collision probe feeds the
crumbling platforms, and where Restart Level returns to.

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

    def test_every_method_is_found(self):
        for name in ("crumble", "spawn"):
            self.assertIsNotNone(getattr(self, name), name)

    def test_the_duck_standing_on_a_platform_comes_from_the_bottom_probe(self):
        self.assertIn("_player.probe_bottom(Cfg::Layer::PLATFORM)", self.crumble)
        self.assertIn("_dynamic_platforms.sync_crumbling(", self.crumble)

    def test_the_level_spawn_is_remembered_for_restart(self):
        self.assertIn("_spawn_x = level.player_data.x", self.spawn)


if __name__ == "__main__":
    unittest.main()
