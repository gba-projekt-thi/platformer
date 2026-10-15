"""Unit tests for tools/Generate_dash_sound.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import tempfile
import unittest
import wave
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Generate_dash_sound.py")
_spec = importlib.util.spec_from_file_location("generate_dash_sound", _SCRIPT)
gen = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gen)


def render():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "dash.wav"
        gen.write(path)
        with wave.open(str(path), "rb") as wav:
            params = wav.getparams()
            frames = wav.readframes(wav.getnframes())
        return params, frames


class DashSoundTests(unittest.TestCase):
    def test_format_is_8_bit_mono(self):
        params, _ = render()
        self.assertEqual(params.nchannels, 1)
        self.assertEqual(params.sampwidth, 1)
        self.assertEqual(params.framerate, 11025)

    def test_length_is_a_short_effect(self):
        params, _ = render()
        seconds = params.nframes / params.framerate
        self.assertGreater(seconds, 0.1)
        self.assertLess(seconds, 0.35)

    def test_is_audible_but_not_clipping(self):
        _, frames = render()
        peak = max(abs(b - 128) for b in frames)
        self.assertGreater(peak, 90)   # loud enough
        self.assertLess(peak, 127)     # never hits the 8-bit rails

    def test_starts_and_ends_near_silence_so_it_does_not_click(self):
        _, frames = render()
        self.assertLessEqual(abs(frames[0] - 128), 8)
        self.assertLessEqual(max(abs(b - 128) for b in frames[-40:]), 12)

    def test_is_deterministic(self):
        self.assertEqual(render()[1], render()[1])

    def test_energy_decays(self):
        _, frames = render()
        half = len(frames) // 2
        first = sum(abs(b - 128) for b in frames[:half])
        second = sum(abs(b - 128) for b in frames[half:])
        self.assertGreater(first, second)


if __name__ == "__main__":
    unittest.main()
