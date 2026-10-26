"""Unit tests for tools/replays.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import re
import struct
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("replays.py")
_spec = importlib.util.spec_from_file_location("replays", _SCRIPT)
replays = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(replays)

REPO = Path(__file__).resolve().parent.parent


def sav_with(blocks):
    """A 32 KB save with recordings {level index: (runs, flags)}."""
    data = bytearray(b"\xff" * 0x8000)
    for index, (runs, flags) in blocks.items():
        offset = replays.SRAM_OFFSET + index * replays.BLOCK_SIZE
        packed = b"".join(struct.pack("<H", (b << 11) | c) for b, c in runs)
        data[offset:offset + 4 + len(packed)] = struct.pack("<BBH", replays.MAGIC, flags, len(runs)) + packed
    return bytes(data)


class LayoutTests(unittest.TestCase):
    def test_layout_matches_replay_h(self):
        text = (REPO / "game/include/replay.h").read_text()
        for name in ("SRAM_OFFSET", "BLOCK_SIZE", "MAGIC"):
            value = re.search(rf"\b{name} = (0x[0-9A-Fa-f]+|\d+);", text).group(1)
            self.assertEqual(int(value, 0), getattr(replays, name), name)


class ImportTests(unittest.TestCase):
    def test_recorded_levels_are_read_back(self):
        sav = sav_with({0: ([(2, 30), (10, 4)], 0), 3: ([(0, 7)], 0)})
        recordings = replays.read_sav(sav, 5)
        self.assertEqual(recordings, {0: ([(2, 30), (10, 4)], False), 3: ([(0, 7)], False)})

    def test_truncated_recordings_are_flagged(self):
        sav = sav_with({1: ([(2, 5)], 0x01)})
        self.assertEqual(replays.read_sav(sav, 5), {1: ([(2, 5)], True)})

    def test_blocks_without_the_magic_are_empty(self):
        self.assertEqual(replays.read_sav(b"\x00" * 0x8000, 5), {})
        self.assertEqual(replays.read_sav(b"\xff" * 0x8000, 5), {})


class GenerateTests(unittest.TestCase):
    def test_header_lists_every_level_in_play_order(self):
        text = replays.generate(["LEVEL_0", "LEVEL_1"], {"LEVEL_1": [[2, 30], [10, 4]]})
        self.assertIn('{"LEVEL_0", nullptr, 0, 0}', text)
        self.assertIn("constexpr replay::Run LEVEL_1_RUNS[] = {{2, 30}, {10, 4}};", text)
        self.assertIn('{"LEVEL_1", LEVEL_1_RUNS, 2, 34}', text)
        self.assertLess(text.index('"LEVEL_0"'), text.index('{"LEVEL_1", LEVEL'))

    def test_header_without_any_replay_still_compiles_to_a_table(self):
        text = replays.generate(["LEVEL_0"], {})
        self.assertIn('{"LEVEL_0", nullptr, 0, 0}', text)


class WriteHeaderTests(unittest.TestCase):
    def setUp(self):
        self.saved = replays.HEADER
        self.dir = tempfile.TemporaryDirectory()
        replays.HEADER = Path(self.dir.name) / "replay_data.h"

    def tearDown(self):
        replays.HEADER = self.saved
        self.dir.cleanup()

    def test_a_header_that_only_differs_in_whitespace_is_kept(self):
        # clang-format reflows the committed header; `make replay` must not
        # undo that and leave the working tree dirty.
        text = replays.generate(replays._campaign_levels(), replays.load_replays())
        reflowed = text.replace("};", "\n};")
        replays.HEADER.write_text(reflowed)
        self.assertEqual(replays.write_header(check=False), 0)
        self.assertEqual(replays.HEADER.read_text(), reflowed)

    def test_a_changed_header_is_rewritten(self):
        replays.HEADER.write_text("// stale\n")
        self.assertEqual(replays.write_header(check=False), 0)
        self.assertIn("ENTRY_COUNT", replays.HEADER.read_text())


class LogTests(unittest.TestCase):
    LOG = """GBA DMA: Starting DMA 1
GBA Debug: REPLAY 0 LEVEL_0 pass 812 900
GBA Debug: REPLAY 1 LEVEL_1 none
GBA Debug: REPLAY 2 LEVEL_WORLD1_SCROLL FAIL 1200 1320
GBA Debug: REPLAY DONE 1 1 1
"""

    def test_results_are_read_from_the_log(self):
        results, done = replays.parse_log(self.LOG)
        self.assertTrue(done)
        self.assertEqual(
            results,
            [("LEVEL_0", "pass"), ("LEVEL_1", "none"), ("LEVEL_WORLD1_SCROLL", "FAIL")],
        )

    def test_a_run_that_never_finished_is_not_done(self):
        _, done = replays.parse_log("GBA Debug: REPLAY 0 LEVEL_0 pass 812 900\n")
        self.assertFalse(done)


if __name__ == "__main__":
    unittest.main()
