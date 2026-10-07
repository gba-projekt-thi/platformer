"""Unit tests for tools/Check_world_index.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Check_world_index.py")
_spec = importlib.util.spec_from_file_location("check_world_index", _SCRIPT)
checker = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(checker)

MAIN = """
int main() {
    const LevelData levels[] = {
        LEVEL_A, LEVEL_B,   // world 1
        LEVEL_C,            // world 2
        LEVEL_D};
}
"""


def worlds(*entries):
    body = ",\n".join(f'    {{"World {i + 1}", {s}, {c}}}' for i, (s, c) in enumerate(entries))
    return f"inline constexpr WorldEntry WORLDS[] = {{\n{body},\n}};\n"


class WorldIndexTests(unittest.TestCase):
    def test_consistent_index_has_no_errors(self):
        errors, warnings = checker.check(MAIN, worlds((0, 2), (2, 2)))
        self.assertEqual((errors, warnings), ([], []))

    def test_parses_levels_ignoring_comments(self):
        self.assertEqual(
            checker.parse_levels(MAIN), ["LEVEL_A", "LEVEL_B", "LEVEL_C", "LEVEL_D"]
        )

    def test_world_count_not_covering_levels_is_an_error(self):
        errors, _ = checker.check(MAIN, worlds((0, 2), (2, 1)))
        self.assertEqual(len(errors), 1)
        self.assertIn("cover 3", errors[0])

    def test_gap_between_worlds_is_an_error(self):
        errors, _ = checker.check(MAIN, worlds((0, 2), (3, 1)))
        self.assertTrue(any("start_index is 3" in e for e in errors))

    def test_overlapping_worlds_are_an_error(self):
        errors, _ = checker.check(MAIN, worlds((0, 3), (2, 2)))
        self.assertTrue(any("start_index is 2" in e for e in errors))

    def test_first_world_must_start_at_zero(self):
        errors, _ = checker.check(MAIN, worlds((1, 2), (3, 1)))
        self.assertTrue(any("expected 0" in e for e in errors))

    def test_duplicate_level_is_an_error(self):
        main = MAIN.replace("LEVEL_D", "LEVEL_A")
        errors, _ = checker.check(main, worlds((0, 2), (2, 2)))
        self.assertTrue(any("LEVEL_A more than once" in e for e in errors))

    def test_defined_but_unlisted_level_is_a_warning(self):
        with tempfile.TemporaryDirectory() as tmp:
            Path(tmp, "levels_world1.h").write_text(
                "const LevelData LEVEL_A = {};\nconst LevelData LEVEL_E = {};\n"
            )
            _, warnings = checker.check(MAIN, worlds((0, 2), (2, 2)), tmp)
        self.assertEqual(len(warnings), 1)
        self.assertIn("LEVEL_E", warnings[0])

    def test_real_roster_shape_matches(self):
        # The real main.cpp uses the same constructs (comments, trailing
        # entries, multi-line list) as the fixture above.
        main = "const LevelData levels[] = {\n LEVEL_0,\n LEVEL_1};\n"
        errors, _ = checker.check(main, worlds((0, 2)))
        self.assertEqual(errors, [])


class DiscoveryTests(unittest.TestCase):
    """The repository has several main.cpp files; the right one must be found."""

    def make_tree(self, root):
        # A decoy under extern/ that sorts BEFORE game/ and has no levels[].
        decoy = Path(root, "extern", "engine", "butano", "examples", "src")
        decoy.mkdir(parents=True)
        (decoy / "main.cpp").write_text("int main() { return 0; }\n")
        # Another one outside extern/ without levels[].
        other = Path(root, "aaa_tool", "src")
        other.mkdir(parents=True)
        (other / "main.cpp").write_text("int main() { return 1; }\n")
        game = Path(root, "game", "src")
        game.mkdir(parents=True)
        (game / "main.cpp").write_text(MAIN)
        include = Path(root, "game", "include")
        include.mkdir(parents=True)
        (include / "world_index.h").write_text(worlds((0, 2), (2, 2)))
        levels = Path(root, "game", "include", "level")
        levels.mkdir(parents=True)
        (levels / "levels_world1.h").write_text("const LevelData LEVEL_A = {};\n")

    def test_picks_the_main_cpp_that_contains_levels(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.make_tree(tmp)
            main_path, world_path, levels_dir = checker.discover(Path(tmp))
            self.assertEqual(main_path, Path(tmp, "game", "src", "main.cpp"))
            self.assertEqual(world_path, Path(tmp, "game", "include", "world_index.h"))
            self.assertEqual(levels_dir, Path(tmp, "game", "include", "level"))

    def test_skips_extern_even_if_it_has_levels(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.make_tree(tmp)
            Path(tmp, "extern", "engine", "butano", "examples", "src", "main.cpp").write_text(MAIN)
            main_path, _, _ = checker.discover(Path(tmp))
            self.assertEqual(main_path, Path(tmp, "game", "src", "main.cpp"))

    def test_nothing_found_returns_none(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(checker.discover(Path(tmp)), (None, None, None))

    def test_main_runs_end_to_end_from_the_repo_root(self):
        import contextlib
        import io
        import os

        with tempfile.TemporaryDirectory() as tmp:
            self.make_tree(tmp)
            old = os.getcwd()
            os.chdir(tmp)
            try:
                out = io.StringIO()
                with contextlib.redirect_stdout(out):
                    # LEVEL_B..D are not defined in levels_world1.h -> only
                    # the coverage matters here: 4 levels, worlds cover 4.
                    code = checker.main([])
            finally:
                os.chdir(old)
            self.assertEqual(code, 0)
            self.assertIn("0 error(s)", out.getvalue())


if __name__ == "__main__":
    unittest.main()
