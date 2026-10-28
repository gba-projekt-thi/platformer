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
    """A 32 KB save with recordings {level index: (runs, flags[, deaths])}."""
    data = bytearray(b"\xff" * 0x8000)
    for index, (runs, flags, *deaths) in blocks.items():
        offset = replays.SRAM_OFFSET + index * replays.BLOCK_SIZE
        packed = b"".join(struct.pack("<H", (b << 10) | c) for b, c in runs)
        header = struct.pack("<BBHH", replays.MAGIC, flags, len(runs), deaths[0] if deaths else 0)
        data[offset:offset + replays.HEADER_SIZE + len(packed)] = header + packed
    return bytes(data)


class LayoutTests(unittest.TestCase):
    def test_layout_matches_replay_h(self):
        text = (REPO / "game/include/replay.h").read_text()
        for name in ("SRAM_OFFSET", "BLOCK_SIZE", "HEADER_SIZE", "MAGIC"):
            value = re.search(rf"\b{name} = (0x[0-9A-Fa-f]+|\d+);", text).group(1)
            self.assertEqual(int(value, 0), getattr(replays, name), name)


class ImportTests(unittest.TestCase):
    def test_recorded_levels_are_read_back(self):
        sav = sav_with({0: ([(2, 30), (10, 4), (40, 1023)], 0), 3: ([(0, 7)], 0)})
        recordings = replays.read_sav(sav, 5)
        self.assertEqual(
            recordings,
            {0: ([(2, 30), (10, 4), (40, 1023)], False, 0), 3: ([(0, 7)], False, 0)})

    def test_truncated_recordings_are_flagged(self):
        sav = sav_with({1: ([(2, 5)], 0x01)})
        self.assertEqual(replays.read_sav(sav, 5), {1: ([(2, 5)], True, 0)})

    def test_the_deaths_a_recording_began_with_are_read_back(self):
        # Recorded after three Restart Levels: the level's layout had moved on.
        sav = sav_with({2: ([(2, 5)], 0, 3)})
        self.assertEqual(replays.read_sav(sav, 5), {2: ([(2, 5)], False, 3)})

    def test_the_json_keeps_the_deaths_only_when_there_were_some(self):
        self.assertEqual(
            replays.replay_json("LEVEL_1", [(2, 5)], 0),
            {"level": "LEVEL_1", "runs": [(2, 5)]})
        self.assertEqual(
            replays.replay_json("LEVEL_1", [(2, 5)], 3),
            {"level": "LEVEL_1", "runs": [(2, 5)], "deaths": 3})

    def test_blocks_without_the_magic_are_empty(self):
        self.assertEqual(replays.read_sav(b"\x00" * 0x8000, 5), {})
        self.assertEqual(replays.read_sav(b"\xff" * 0x8000, 5), {})


class GenerateTests(unittest.TestCase):
    def test_header_lists_every_level_in_play_order(self):
        text = replays.generate(["LEVEL_0", "LEVEL_1"], {"LEVEL_1": [[2, 30], [10, 4]]})
        self.assertIn('{"LEVEL_0", nullptr, 0, 0, false, 0, 0, 0}', text)
        self.assertIn("constexpr replay::Run LEVEL_1_RUNS[] = {{2, 30}, {10, 4}};", text)
        self.assertIn('{"LEVEL_1", LEVEL_1_RUNS, 2, 34, false, 0, 0, 0}', text)
        self.assertLess(text.index('"LEVEL_0"'), text.index('{"LEVEL_1", LEVEL'))

    def test_a_replay_may_start_somewhere_else(self):
        # A boss fight is tested on its own: the duck starts in the arena.
        text = replays.generate(
            ["LEVEL_0", "LEVEL_1"],
            {"LEVEL_1": [[2, 30]]}, {"LEVEL_1": [120, 30]})
        self.assertIn('{"LEVEL_1", LEVEL_1_RUNS, 1, 30, true, 120, 30, 0}', text)
        self.assertIn('{"LEVEL_0", nullptr, 0, 0, false, 0, 0, 0}', text)

    def test_a_replay_may_begin_after_deaths_in_the_level(self):
        # The level as it stands after two deaths (death windows, moved door).
        text = replays.generate(
            ["LEVEL_0", "LEVEL_1"], {"LEVEL_1": [[2, 30]]}, {}, {"LEVEL_1": 2})
        self.assertIn("unsigned deaths;", text)
        self.assertIn('{"LEVEL_1", LEVEL_1_RUNS, 1, 30, false, 0, 0, 2}', text)

    def test_counter_checks_get_their_own_table(self):
        # Runs that must NOT reach the door: same layout, separate arrays.
        text = replays.generate(
            ["LEVEL_0", "LEVEL_1"], {"LEVEL_1": [[2, 30]]}, None, None,
            counter=({"LEVEL_1": [[2, 5], [0, 3]]}, {"LEVEL_1": [8, 16]}, {}))
        self.assertIn("constexpr replay::Run LEVEL_1_COUNTER_RUNS[] = {{2, 5}, {0, 3}};", text)
        table = text[text.index("COUNTER_ENTRIES[]"):]
        self.assertIn('{"LEVEL_1", LEVEL_1_COUNTER_RUNS, 2, 8, true, 8, 16, 0}', table)
        self.assertIn('{"LEVEL_0", nullptr, 0, 0, false, 0, 0, 0}', table)
        self.assertIn('{"LEVEL_1", LEVEL_1_RUNS, 1, 30, false, 0, 0, 0}', text[:text.index("COUNTER_ENTRIES[]")])

    def test_every_boss_level_has_a_counter_check(self):
        levels = replays._campaign_levels()
        counter, starts, _ = replays.load_replays(replays.COUNTER_DIR)
        for world in range(1, 6):
            name = f"LEVEL_WORLD{world}_BOSS"
            self.assertIn(name, levels)
            self.assertIn(name, counter)
            self.assertIn(name, starts)
            self.assertFalse(any(b & 32 for b, _ in counter[name]), "no R: the boss must live")

    def test_header_without_any_replay_still_compiles_to_a_table(self):
        text = replays.generate(["LEVEL_0"], {})
        self.assertIn('{"LEVEL_0", nullptr, 0, 0, false, 0, 0, 0}', text)


class LoadTests(unittest.TestCase):
    def setUp(self):
        self.saved = replays.REPLAY_DIR
        self.dir = tempfile.TemporaryDirectory()
        replays.REPLAY_DIR = Path(self.dir.name)

    def tearDown(self):
        replays.REPLAY_DIR = self.saved
        self.dir.cleanup()

    def test_deaths_and_starts_are_optional(self):
        (replays.REPLAY_DIR / "A.json").write_text(
            '{"level": "LEVEL_A", "runs": [[2, 5]], "deaths": 3}')
        (replays.REPLAY_DIR / "B.json").write_text(
            '{"level": "LEVEL_B", "runs": [[1, 4]], "start": [8, 16]}')
        runs, starts, deaths = replays.load_replays()
        self.assertEqual(runs, {"LEVEL_A": [[2, 5]], "LEVEL_B": [[1, 4]]})
        self.assertEqual(starts, {"LEVEL_B": [8, 16]})
        self.assertEqual(deaths, {"LEVEL_A": 3})


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
        text = replays.generate(replays._campaign_levels(), *replays.load_replays())
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

    def test_counter_checks_are_listed_apart(self):
        results, _ = replays.parse_log(
            "GBA Debug: REPLAY 4 LEVEL_WORLD1_BOSS counter pass 80 205\n"
            "GBA Debug: REPLAY 8 LEVEL_WORLD2_BOSS counter FAIL 80 90\n")
        self.assertEqual(
            results, [("LEVEL_WORLD1_BOSS/counter", "pass"), ("LEVEL_WORLD2_BOSS/counter", "FAIL")])

    def test_a_run_that_never_finished_is_not_done(self):
        _, done = replays.parse_log("GBA Debug: REPLAY 0 LEVEL_0 pass 812 900\n")
        self.assertFalse(done)


if __name__ == "__main__":
    unittest.main()
