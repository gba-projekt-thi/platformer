"""Unit tests for tools/Generate_checkpoint_sprite.py.

Run from the repository root:

    python3 -m unittest discover -s tools -p 'test_*.py'
"""

import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

_SCRIPT = Path(__file__).with_name("Generate_checkpoint_sprite.py")
_spec = importlib.util.spec_from_file_location("generate_checkpoint_sprite", _SCRIPT)
gen = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(gen)


def read_bmp(path):
    data = Path(path).read_bytes()
    assert data[:2] == b"BM"
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height, planes, bpp = struct.unpack_from("<iiHH", data, 18)
    colors = struct.unpack_from("<I", data, 46)[0]
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
    return {"width": width, "height": height, "bpp": bpp, "colors": colors,
            "palette": palette, "rows": rows}


class CheckpointSpriteTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        gen.main([cls.tmp.name])
        cls.bmp = read_bmp(Path(cls.tmp.name, "checkpoint16x16.bmp"))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_is_two_stacked_16x16_frames_in_4bpp(self):
        self.assertEqual((self.bmp["width"], self.bmp["height"]), (16, 32))
        self.assertEqual(self.bmp["bpp"], 4)

    def test_palette_index_zero_is_the_transparent_magenta(self):
        self.assertEqual(self.bmp["palette"][0], (255, 0, 255))

    def test_the_two_frames_differ(self):
        rows = self.bmp["rows"]
        self.assertNotEqual(rows[:16], rows[16:])

    def test_both_frames_have_transparent_and_drawn_pixels(self):
        for frame in (self.bmp["rows"][:16], self.bmp["rows"][16:]):
            flat = [p for row in frame for p in row]
            self.assertIn(0, flat)
            self.assertTrue(any(p != 0 for p in flat))

    def test_active_frame_has_green_and_inactive_frame_does_not(self):
        green = gen.GREEN
        inactive = [p for row in self.bmp["rows"][:16] for p in row]
        active = [p for row in self.bmp["rows"][16:] for p in row]
        self.assertNotIn(green, inactive)
        self.assertIn(green, active)

    def test_pole_stands_in_the_same_place_in_both_frames(self):
        # the pole column is the same, only the flag moves
        for y in range(2, 13):
            self.assertEqual(self.bmp["rows"][y][7], self.bmp["rows"][16 + y][7])

    def test_json_matches_the_butano_sprite_format(self):
        meta = json.loads(Path(self.tmp.name, "checkpoint16x16.json").read_text())
        self.assertEqual(meta, {"type": "sprite", "height": 16})


if __name__ == "__main__":
    unittest.main()
