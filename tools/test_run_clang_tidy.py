"""Unit tests for tools/run_clang_tidy.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("run_clang_tidy.py")
_spec = importlib.util.spec_from_file_location("run_clang_tidy", _SCRIPT)
tidy = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(tidy)

ROOT = "/wt"
SOURCE = {
    "arguments": ["gcc", "-std=c++23", "-I/wt/game/include", "-c", "-o", "a.o", "/wt/game/src/a.cpp"],
    "directory": "/wt/build",
    "file": "/wt/game/src/a.cpp",
}


class RewriteRootTests(unittest.TestCase):
    def test_every_path_moves_to_the_new_root(self):
        text = '{"file": "/workspace/a.cpp", "arguments": ["-I/workspace/x"]}'
        self.assertEqual(
            tidy.rewrite_root(text, "/workspace", "/wt"),
            '{"file": "/wt/a.cpp", "arguments": ["-I/wt/x"]}',
        )

    def test_same_root_changes_nothing(self):
        self.assertEqual(tidy.rewrite_root("/a/b", "/a", "/a"), "/a/b")


class MissingEntriesTests(unittest.TestCase):
    def test_header_gets_a_cpp_command_from_a_game_source(self):
        db = tidy.add_missing_entries([SOURCE], ["/wt/game/include/x.h"], ROOT)
        self.assertEqual(len(db), 2)
        added = db[1]
        self.assertEqual(added["file"], "/wt/game/include/x.h")
        self.assertEqual(added["arguments"][:3], ["gcc", "-x", "c++"])
        self.assertEqual(added["arguments"][-1], "/wt/game/include/x.h")
        self.assertNotIn("/wt/game/src/a.cpp", added["arguments"])
        self.assertIn("-std=c++23", added["arguments"])

    def test_files_with_an_entry_are_left_alone(self):
        db = tidy.add_missing_entries([SOURCE], ["/wt/game/src/a.cpp"], ROOT)
        self.assertEqual(db, [SOURCE])

    def test_without_a_game_source_the_database_is_unchanged(self):
        self.assertEqual(tidy.add_missing_entries([], ["/wt/x.h"], ROOT), [])


if __name__ == "__main__":
    unittest.main()
