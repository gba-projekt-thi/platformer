"""Unit tests for tools/Upscale_sprite.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

_here = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("upscale_sprite", _here / "Upscale_sprite.py")
up = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(up)
_hb_spec = importlib.util.spec_from_file_location("check_hitboxes", _here / "Check_hitboxes.py")
hb = importlib.util.module_from_spec(_hb_spec)
_hb_spec.loader.exec_module(hb)


class UpscaleTests(unittest.TestCase):
    def make(self, tmp, rows, bpp=4):
        src = Path(tmp) / "fish8x8.bmp"
        palette = bytes(range(4 * (1 << bpp)))[: 4 * (1 << bpp)] if bpp == 4 else None
        hb.write_bmp(src, rows, bpp=bpp, palette=palette)
        (Path(tmp) / "fish8x8.json").write_text('{"type": "sprite", "height": 8}\n')
        return src

    def test_every_pixel_becomes_a_2x2_block(self):
        rows = [[(x + y) % 3 for x in range(8)] for y in range(16)]  # two 8x8 frames
        with tempfile.TemporaryDirectory() as tmp:
            src = self.make(tmp, rows)
            out = up.upscale(src, Path(tmp) / "fish16x16.bmp")
            big = hb.read_bmp(out)
        self.assertEqual((len(big[0]), len(big)), (16, 32))
        for y in range(32):
            for x in range(16):
                self.assertEqual(big[y][x], rows[y // 2][x // 2])

    def test_palette_and_depth_are_kept(self):
        rows = [[1] * 8 for _ in range(8)]
        with tempfile.TemporaryDirectory() as tmp:
            src = self.make(tmp, rows)
            out = up.upscale(src, Path(tmp) / "fish16x16.bmp")
            self.assertEqual(up.palette_of(out), up.palette_of(src))
            self.assertEqual(up.bpp_of(out), 4)

    def test_the_json_frame_height_doubles(self):
        rows = [[0] * 8 for _ in range(8)]
        with tempfile.TemporaryDirectory() as tmp:
            src = self.make(tmp, rows)
            up.upscale(src, Path(tmp) / "fish16x16.bmp")
            data = json.loads((Path(tmp) / "fish16x16.json").read_text())
        self.assertEqual(data, {"type": "sprite", "height": 16})


if __name__ == "__main__":
    unittest.main()
