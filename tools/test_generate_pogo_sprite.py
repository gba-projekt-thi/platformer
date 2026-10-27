"""Unit tests for tools/Generate_pogo_sprite.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Generate_pogo_sprite.py")
_spec = importlib.util.spec_from_file_location("generate_pogo_sprite", _SCRIPT)
gen = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gen)


def read_bmp(path):
    data = Path(path).read_bytes()
    assert data[:2] == b"BM"
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height, planes, bpp = struct.unpack_from("<iiHH", data, 18)
    palette = [tuple(data[54 + 4 * i : 57 + 4 * i][::-1]) for i in range(16)]
    row_bytes = ((width * bpp + 31) // 32) * 4
    rows = []
    for y in range(height - 1, -1, -1):  # BMP is stored bottom-up
        start = offset + y * row_bytes
        row = []
        for x in range(width):
            byte = data[start + x // 2]
            row.append(byte >> 4 if x % 2 == 0 else byte & 0xF)
        rows.append(row)
    return {"width": width, "height": height, "bpp": bpp,
            "palette": palette, "rows": rows}


class PogoSpriteTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        gen.main([cls.tmp.name, str(Path(cls.tmp.name, "preview.png"))])
        cls.bmp = read_bmp(Path(cls.tmp.name, "pogo16x16.bmp"))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def frames(self):
        rows = self.bmp["rows"]
        return [rows[i * 16 : (i + 1) * 16] for i in range(3)]

    def test_is_three_stacked_16x16_frames_in_4bpp(self):
        self.assertEqual((self.bmp["width"], self.bmp["height"]), (16, 48))
        self.assertEqual(self.bmp["bpp"], 4)

    def test_palette_index_zero_is_the_transparent_magenta(self):
        self.assertEqual(self.bmp["palette"][0], (255, 0, 255))

    def test_frames_differ_and_have_transparent_and_drawn_pixels(self):
        frames = self.frames()
        self.assertEqual(len({str(f) for f in frames}), 3)
        for frame in frames:
            flat = [p for row in frame for p in row]
            self.assertIn(0, flat)
            self.assertTrue(any(p != 0 for p in flat))

    def test_every_pixel_uses_a_defined_palette_entry(self):
        used = {p for row in self.bmp["rows"] for p in row}
        self.assertTrue(used <= set(gen.PALETTE))

    def test_impact_star_only_in_the_later_frames(self):
        first, second, third = ([p for row in f for p in row] for f in self.frames())
        self.assertNotIn(gen.YELLOW, first)
        self.assertIn(gen.YELLOW, second)
        self.assertIn(gen.YELLOW, third)

    def test_preview_png_is_written(self):
        png = Path(self.tmp.name, "preview.png").read_bytes()
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")

    def test_json_matches_the_butano_sprite_format(self):
        meta = json.loads(Path(self.tmp.name, "pogo16x16.json").read_text())
        self.assertEqual(meta, {"type": "sprite", "height": 16})


if __name__ == "__main__":
    unittest.main()
