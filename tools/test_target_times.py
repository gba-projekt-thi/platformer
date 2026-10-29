"""Unit tests for tools/Target_times.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Target_times.py")
_spec = importlib.util.spec_from_file_location("target_times", _SCRIPT)
tt = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(tt)


def replays(files):
    directory = tempfile.TemporaryDirectory()
    for name, data in files.items():
        (Path(directory.name) / name).write_text(json.dumps(data))
    return directory


class TargetTimesTest(unittest.TestCase):
    def test_target_is_one_and_a_half_times_rounded_up_to_whole_seconds(self):
        self.assertEqual(tt.target_for(240), 6)  # 4 s -> 6 s exactly
        self.assertEqual(tt.target_for(241), 7)
        self.assertEqual(tt.target_for(1), 1)

    def test_proof_frames_add_the_walk_to_a_recordings_start(self):
        with replays({"A.json": {"level": "A", "runs": [[0, 100]]},
                      "B.json": {"level": "B", "runs": [[0, 100]], "start": [100, 0]}}) as d:
            level = {"spawn": [-100, 0]}
            self.assertEqual(tt.proof_frames({"name": "A", **level}, d, 2), 100)
            self.assertEqual(tt.proof_frames({"name": "B", **level}, d, 2), 200)
            self.assertIsNone(tt.proof_frames({"name": "C", **level}, d, 2))

    def test_problems_reports_a_missing_or_unbeatable_target(self):
        with replays({"A.json": {"level": "A", "runs": [[0, 600]]}}) as d:
            level = {"name": "A", "spawn": [0, 0]}
            self.assertTrue(tt.problems(level, d, 2))
            self.assertTrue(tt.problems({**level, "target_time_s": 9}, d, 2))
            self.assertFalse(tt.problems({**level, "target_time_s": 10}, d, 2))
            self.assertTrue(tt.problems({"name": "Z", "target_time_s": 99}, d, 2))

    def test_write_targets_inserts_once_and_replaces(self):
        text = '{\n      "name": "A",\n      "music": "m",\n      "spawn": [0, 0]\n}\n'
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "world.json"
            path.write_text(text)
            once = tt.write_targets(path, {"A": 7})
            self.assertIn('      "music": "m",\n      "target_time_s": 7,\n', once)
            path.write_text(once)
            self.assertEqual(tt.write_targets(path, {"A": 7}), once)
            self.assertIn('"target_time_s": 9', tt.write_targets(path, {"A": 9}))
            self.assertNotIn('"target_time_s": 7', tt.write_targets(path, {"A": 9}))


if __name__ == "__main__":
    unittest.main()
