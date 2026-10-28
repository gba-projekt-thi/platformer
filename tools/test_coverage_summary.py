"""Unit tests for tools/coverage_summary.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("coverage_summary.py")
_spec = importlib.util.spec_from_file_location("coverage_summary", _SCRIPT)
cov = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(cov)


def report(cwd, *files):
    """One object's gcov JSON: files as (path, {line: count})."""
    return {
        "current_working_directory": cwd,
        "files": [
            {"file": path, "lines": [{"line_number": n, "count": c} for n, c in lines.items()]}
            for path, lines in files
        ],
    }


class MergeTests(unittest.TestCase):
    def test_a_line_counts_as_covered_if_any_object_ran_it(self):
        merged = cov.merge([
            report("/r/tests/host", ("../../game/include/a.h", {1: 0, 2: 3})),
            report("/r/tests/host", ("../../game/include/a.h", {1: 5, 2: 0})),
        ], "/r/game")
        self.assertEqual(merged, {"include/a.h": (2, 2)})

    def test_lines_never_run_stay_uncovered(self):
        merged = cov.merge([report("/r/tests/host", ("../../game/include/a.h", {1: 0, 2: 1}))], "/r/game")
        self.assertEqual(merged, {"include/a.h": (1, 2)})

    def test_files_outside_the_root_are_ignored(self):
        merged = cov.merge([
            report("/r/tests/host", ("test_a.cpp", {1: 1}), ("/usr/include/c++/vector", {1: 1})),
        ], "/r/game")
        self.assertEqual(merged, {})

    def test_summary_lists_least_covered_first_with_a_total(self):
        lines = cov.summary({"include/a.h": (1, 4), "include/b.h": (3, 3)})
        self.assertIn("include/a.h", lines[0])
        self.assertIn(" 25.0%", lines[0])
        self.assertIn("include/b.h", lines[1])
        self.assertIn("4/7", lines[-1])


if __name__ == "__main__":
    unittest.main()
