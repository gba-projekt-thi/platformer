"""Tests for post-create.sh and its dev container wiring.

The script runs when the dev container is created. It once ran twice - as
postCreateCommand and as updateContentCommand - and each run did 'make clean'
plus a full build under bear. These tests run the script against stand-ins for
pre-commit, make and bear (nothing is built) and pin the devcontainer.json
wiring down. Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import os
import re
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "post-create.sh"
DEVCONTAINER = ROOT / ".devcontainer" / "devcontainer.json"

# What bear writes: a compile command with the flag clang-tidy cannot handle.
COMPILE_DB = '[{"arguments": ["g++", "-mthumb-interwork", "-c", "a.cpp"], "file": "a.cpp"}]\n'


def _stub(path, body):
    path.write_text("#!/bin/bash\n" + body)
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


class Sandbox:
    """A temporary workspace with stand-ins for pre-commit, make and bear that
    log every call to calls.log."""

    def __init__(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self._tmp.name)
        self.bin = self.dir / "bin"
        self.bin.mkdir()
        self.log = self.dir / "calls.log"
        _stub(self.bin / "pre-commit", 'echo "pre-commit $*" >> "$CALLS_LOG"\n')
        _stub(self.bin / "make", 'echo "make $*" >> "$CALLS_LOG"\n')
        # bear -- make -j1: log it, run the build command, write the database
        _stub(
            self.bin / "bear",
            'echo "bear $*" >> "$CALLS_LOG"\nshift\n"$@"\n'
            f"cat > compile_commands.json <<'EOF'\n{COMPILE_DB}EOF\n",
        )
        (self.dir / "post-create.sh").write_text(SCRIPT.read_text())
        (self.dir / "post-create.sh").chmod(0o755)

    def run(self, *args):
        env = dict(os.environ, PATH=f"{self.bin}:{os.environ['PATH']}", CALLS_LOG=str(self.log))
        result = subprocess.run(
            ["bash", "./post-create.sh", *args], cwd=self.dir, env=env, capture_output=True, text=True
        )
        self.last = result
        return result

    def calls(self):
        return self.log.read_text().splitlines() if self.log.exists() else []

    def reset_calls(self):
        if self.log.exists():
            self.log.unlink()

    @property
    def db(self):
        return self.dir / "compile_commands.json"

    def close(self):
        self._tmp.cleanup()


@unittest.skipIf(not SCRIPT.exists(), "post-create.sh not found")
class PostCreateScriptTests(unittest.TestCase):
    def setUp(self):
        self.box = Sandbox()
        self.addCleanup(self.box.close)

    def test_first_run_installs_the_hook_and_builds_once_under_bear(self):
        result = self.box.run()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            self.box.calls(),
            ["pre-commit install", "make clean", "bear -- make -j1", "make -j1"],
        )

    def test_the_database_is_cleaned_for_clang_tidy(self):
        self.box.run()
        text = self.box.db.read_text()
        self.assertNotIn("-mthumb-interwork", text)
        self.assertIn('"file": "a.cpp"', text)

    def test_a_second_run_does_not_build_again(self):
        self.box.run()
        self.box.reset_calls()
        result = self.box.run()
        self.assertEqual(result.returncode, 0, result.stderr)
        # the hook install is cheap and idempotent; the full build is not repeated
        self.assertEqual(self.box.calls(), ["pre-commit install"])
        self.assertIn("skipping the build", result.stdout)

    def test_an_existing_database_is_left_untouched(self):
        self.box.db.write_text("[]\n")
        self.box.run()
        self.assertEqual(self.box.db.read_text(), "[]\n")
        self.assertNotIn("make clean", self.box.calls())

    def test_an_empty_database_counts_as_missing(self):
        self.box.db.write_text("")
        self.box.run()
        self.assertIn("make clean", self.box.calls())
        self.assertTrue(self.box.db.read_text().strip())

    def test_force_regenerates_it(self):
        self.box.run()
        self.box.reset_calls()
        self.box.run("--force")
        self.assertEqual(
            self.box.calls(),
            ["pre-commit install", "make clean", "bear -- make -j1", "make -j1"],
        )

    def test_make_clean_always_comes_right_before_the_bear_build(self):
        # bear only records what is compiled, so the tree has to be cleaned first
        self.box.run()
        calls = self.box.calls()
        self.assertEqual(calls.index("bear -- make -j1"), calls.index("make clean") + 1)

    def test_it_never_builds_without_bear(self):
        self.box.run()
        self.box.reset_calls()
        self.box.db.unlink()
        self.box.run()
        calls = self.box.calls()
        self.assertEqual(calls.count("make clean"), 1)
        # the plain 'make -j1' in the log is the one bear started
        self.assertEqual(calls.count("make -j1"), 1)
        self.assertLess(calls.index("bear -- make -j1"), calls.index("make -j1"))


@unittest.skipIf(not DEVCONTAINER.exists(), "devcontainer.json not found")
class DevContainerWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = DEVCONTAINER.read_text()
        # JSON with comments: only the settings lines matter here
        cls.settings = [line for line in text.splitlines() if not line.strip().startswith("//")]
        cls.text = "\n".join(cls.settings)

    def test_post_create_runs_as_post_create_command(self):
        self.assertRegex(self.text, r'"postCreateCommand"\s*:\s*"[^"]*post-create\.sh"')

    def test_it_is_not_also_the_update_content_command(self):
        # Both hooks run when the container is created: that built the project
        # twice, once as 'make' and once as 'make clean; make'.
        self.assertNotIn("updateContentCommand", self.text)
        self.assertEqual(len(re.findall(r"post-create\.sh", self.text)), 1)


if __name__ == "__main__":
    unittest.main()
