"""Unit tests for tools/xmfile.py and tools/Make_boss_music.py.

A small synthetic module (not the game's multi-megabyte ones), then the real
modules and level JSON for consistency. Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import xmfile  # noqa: E402
from xmfile import Note, Pattern  # noqa: E402

_SCRIPT = Path(__file__).with_name("Make_boss_music.py")
_spec = importlib.util.spec_from_file_location("make_boss_music", _SCRIPT)
mbm = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(mbm)


def instrument(volume=64, samples=64):
    """An instrument with one 8-bit sample of `samples` bytes."""
    header = struct.pack("<I22sBH", 263, b"test", 0, 1)
    header += struct.pack("<I", 40).ljust(263 - 29, b"\0")
    sample = struct.pack("<IIIBbBBbB22s", samples, 0, 0, volume, 0, 0, 128, 0, 0, b"s")
    return header + sample + bytes(samples)


def synthetic(patterns=6, rows=32, restart=0):
    """Bass on channel 0, melody on channel 1, kick on channel 2; the
    patterns get busier towards the middle."""
    pats = []
    for p in range(patterns):
        grid = [[Note() for _ in range(8)] for _ in range(rows)]
        for r in range(0, rows, 8):
            grid[r][0] = Note(25, 1)  # low bass, long gaps
        for r in range(0, rows, max(1, 8 - 2 * min(p, patterns - 1 - p))):
            grid[r][1] = Note(60, 2)
        for r in range(0, rows, 4):
            grid[r][2] = Note(49, 3)
        pats.append(Pattern(grid))
    return xmfile.Module(
        b"test", b"unit", 0x104, restart, 8, 1, 6, 100, list(range(patterns)),
        pats, [instrument(), instrument(), instrument(40)])


PRESET = {"kick": 3, "hat": None}


def effects(row):
    return [(n.fx, n.param) for n in row if n.fx]


class XmFileTest(unittest.TestCase):
    def test_round_trip(self):
        mod = synthetic()
        parsed = xmfile.parse(xmfile.write(mod))
        self.assertEqual(parsed.patterns, mod.patterns)
        self.assertEqual(parsed.instruments, mod.instruments)
        self.assertEqual(parsed.order, mod.order)
        self.assertEqual(xmfile.write(parsed), xmfile.write(mod))

    def test_sample_bytes(self):
        mod = synthetic()
        self.assertEqual(xmfile.sample_total(mod), 64 * 3)
        self.assertEqual(xmfile.used_instruments(mod), {1, 2, 3})


class BuildTest(unittest.TestCase):
    def setUp(self):
        self.theme = synthetic()
        self.boss = mbm.build(self.theme, PRESET)

    def test_boss_section_after_the_theme(self):
        self.assertEqual(mbm.boss_start(self.boss), 6)
        self.assertEqual(self.boss.order[:6], list(range(6)))
        self.assertGreater(len(self.boss.order), 6)

    def test_samples_are_shared_not_added(self):
        self.assertEqual(self.boss.instruments, self.theme.instruments)

    def test_theme_notes_untouched(self):
        for o in range(6):
            for row_a, row_b in zip(self.theme.patterns[o].rows,
                                    self.boss.patterns[o].rows):
                for c in range(8):
                    a, b = row_a[c], row_b[c]
                    self.assertEqual((a.note, a.inst, a.vol), (b.note, b.inst, b.vol))

    def test_loops(self):
        theme_last = self.boss.patterns[self.boss.order[5]].rows[-1]
        boss_last = self.boss.patterns[self.boss.order[-1]].rows[-1]
        self.assertIn((mbm.FX_JUMP, 0), effects(theme_last))
        self.assertIn((mbm.FX_JUMP, 6), effects(boss_last))

    def test_restart_order_is_kept(self):
        boss = mbm.build(synthetic(restart=1), PRESET)
        last = boss.patterns[boss.order[5]].rows[-1]
        self.assertIn((mbm.FX_JUMP, 1), effects(last))

    def test_tempo_faster_in_boss_and_restored_in_theme(self):
        def tempo(order):
            row = self.boss.patterns[self.boss.order[order]].rows[0]
            return [p for fx, p in effects(row) if fx == mbm.FX_TEMPO]
        self.assertEqual(tempo(0), [100])
        self.assertEqual(tempo(6), [125])

    def test_boss_is_busier_than_the_theme(self):
        def density(orders):
            return sum(mbm.energy(self.boss, self.boss.patterns[self.boss.order[o]])
                       for o in orders) / len(orders)
        self.assertGreater(density(range(6, len(self.boss.order))),
                           density(range(6)))

    def test_channels_added_only_when_none_is_free(self):
        # channels 3..7 are empty (the boss layers need four): none is added
        self.assertEqual(self.boss.channels, 8)
        full = synthetic()
        for p in full.patterns:
            for row in p.rows:
                for c in range(3, 8):
                    row[c] = Note(40, 1)
        self.assertEqual(mbm.build(full, PRESET).channels, 12)

    def test_idempotent_and_strip_restores_the_theme(self):
        self.assertEqual(xmfile.write(mbm.build(self.boss, PRESET)),
                         xmfile.write(self.boss))
        self.assertEqual(xmfile.write(mbm.strip(self.boss)),
                         xmfile.write(self.theme))

    def test_drone_sits_low_and_the_last_pattern_gets_a_tritone(self):
        preset = dict(PRESET, drone=1)
        boss = mbm.build(self.theme, preset)
        first = boss.patterns[boss.order[6]].rows
        last = boss.patterns[boss.order[-1]].rows
        notes = [(r, c, n.note) for r, row in enumerate(first)
                 for c, n in enumerate(row) if c == 4 and n.note]
        self.assertEqual([r for r, _, _ in notes], [0])
        self.assertTrue(mbm.DRONE_LOW <= notes[0][2] < mbm.DRONE_LOW + 12)
        stab = [n.note for row in last for n in row[4:5] if n.note]
        self.assertEqual(stab[1] - stab[0], 6)

    def test_no_boss_section_in_a_plain_module(self):
        self.assertIsNone(mbm.boss_start(self.theme))


MAX_MUSIC_CHANNELS = 16  # BN_CFG_AUDIO_MAX_MUSIC_CHANNELS


class RealFilesTest(unittest.TestCase):
    def modules(self):
        for name in mbm.PRESETS:
            path = mbm.AUDIO / f"{name}.xm"
            if path.exists():
                yield name, xmfile.parse(path.read_bytes())

    def test_only_what_maxmod_plays_in_range(self):
        for name, mod in self.modules():
            self.assertLessEqual(mod.channels, MAX_MUSIC_CHANNELS, name)
            for order, pi in enumerate(mod.order):
                rows = mod.patterns[pi].rows
                self.assertLessEqual(len(rows), 256, name)
                for row in rows:
                    for n in row:
                        self.assertLessEqual(n.note, mbm.KEY_OFF, name)
                        self.assertLessEqual(n.inst, len(mod.instruments), name)
                        if n.fx == mbm.FX_JUMP:
                            self.assertLess(n.param, len(mod.order), name)
                        elif n.fx == mbm.FX_TEMPO:
                            self.assertGreaterEqual(n.param, 32, name)

    def test_every_boss_section_loops_and_the_theme_returns(self):
        for name, mod in self.modules():
            start = mbm.boss_start(mod)
            jumps = {o: [n.param for n in mod.patterns[pi].rows[-1]
                         if n.fx == mbm.FX_JUMP] for o, pi in enumerate(mod.order)}
            self.assertEqual(jumps[len(mod.order) - 1], [start], name)
            self.assertEqual(jumps[start - 1], [mod.restart], name)

    def test_game_modules_are_up_to_date(self):
        self.assertEqual(mbm.main(["--check"]), 0)


if __name__ == "__main__":
    unittest.main()
