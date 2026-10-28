#!/usr/bin/env python3
"""
Generate_pogo_sprite.py

Writes the three-frame pogo strike graphic (shown under the duck while the
down strike reaches, Player::_update_pogo_slash) as a Butano-ready 4bpp
indexed BMP plus its JSON. It is the duck's beak dive: a yellow teardrop with
an orange beak tip plunges down, then splashes into the bathtub's water.

    pogo16x16.bmp   16 x 48 px, three 16x16 frames stacked vertically
                    frame 0 = the beak dive starts, water streaks beside it
                    frame 1 = it plunges lower, a ripple forms under the tip
                    frame 2 = the dive is a small tip, a splash crown of
                              droplets and a water line below
    pogo16x16.json  {"type": "sprite", "height": 16}

The look follows ente.bmp: black outline, yellow body, cream highlight,
orange beak. Water is light blue. Palette index 0 is the transparent color
(magenta), as Butano expects. Each frame is drawn as its left half and
mirrored. Pure Python (no Pillow). Usage:

    python3 tools/Generate_pogo_sprite.py game/graphics/global [preview.png]

The optional second argument also writes an enlarged PNG preview.
The sprite is blended in game, so it stays light and airy.
"""

import struct
import sys
import zlib
from pathlib import Path

W, H, FRAMES = 16, 16, 3
T = 0  # transparent
OUT, YELLOW, CREAM, ORANGE, WATER, WHITE = 1, 2, 3, 4, 5, 6

# Same colors as ente.bmp (outline, yellow body, cream highlight, orange
# beak) plus a light water blue and white foam.
PALETTE = {
    T: (255, 0, 255),
    OUT: (0, 0, 0),
    YELLOW: (255, 236, 39),
    CREAM: (255, 241, 232),
    ORANGE: (255, 163, 0),
    WATER: (120, 190, 240),
    WHITE: (255, 255, 255),
}

GLYPHS = {".": T, "k": OUT, "y": YELLOW, "c": CREAM, "o": ORANGE,
          "b": WATER, "w": WHITE}

# Left half (8 columns) of each frame, 16 rows; mirrored to the right.
HALVES = (
    (  # 0: the dive starts
        "........",
        "........",
        "......kk",
        ".....kyc",
        "b...kycy",
        "b...kycy",
        ".b..kyyy",
        ".b...kyy",
        "..b..kyy",
        ".....koo",
        "......ko",
        "......kk",
        "........",
        "........",
        "........",
        "........",
    ),
    (  # 1: it plunges, a ripple forms
        "........",
        "........",
        "........",
        "......kk",
        ".....kyc",
        "b...kycy",
        "b...kycy",
        ".b..kyyy",
        ".b...kyy",
        "..b..kyy",
        ".....koo",
        "...b..ko",
        "..bbb.kk",
        ".bbbbbbb",
        "........",
        "........",
    ),
    (  # 2: a splash crown
        "........",
        "........",
        "........",
        "......kk",
        ".....kyo",
        "b...b.ko",
        ".b..b..k",
        ".b.b.b..",
        "..b.b..b",
        "w.b.bwb.",
        ".bbbbbbb",
        "bbbwwbbb",
        "........",
        "........",
        "........",
        "........",
    ),
)


def frame(index):
    img = []
    for half in HALVES[index]:
        assert len(half) == W // 2
        left = [GLYPHS[c] for c in half]
        img.append(left + left[::-1])
    return img


def frames():
    rows = []
    for i in range(FRAMES):
        rows += frame(i)
    return rows


def write_bmp(path, rows):
    height, width = len(rows), len(rows[0])
    row_bytes = ((width * 4 + 31) // 32) * 4
    pixel_data = bytearray()
    for y in range(height - 1, -1, -1):  # BMP is bottom-up
        row = bytearray(row_bytes)
        for x in range(0, width, 2):
            hi = rows[y][x]
            lo = rows[y][x + 1] if x + 1 < width else 0
            row[x // 2] = (hi << 4) | lo
        pixel_data += row
    palette = bytearray()
    for i in range(16):
        r, g, b = PALETTE.get(i, (0, 0, 0))
        palette += bytes((b, g, r, 0))
    offset = 14 + 40 + len(palette)
    header = b"BM" + struct.pack("<IHHI", offset + len(pixel_data), 0, 0, offset)
    info = struct.pack(
        "<IiiHHIIiiII", 40, width, height, 1, 4, 0, len(pixel_data), 2835, 2835, 16, 16
    )
    Path(path).write_bytes(header + info + bytes(palette) + bytes(pixel_data))


def write_preview(path, rows, scale=16):
    """Frames side by side on a sky-blue checker, enlarged, as PNG."""
    n = len(rows) // H
    pw, ph = n * (W + 1) * scale, H * scale
    raw = bytearray()
    for py in range(ph):
        raw.append(0)
        y = py // scale
        for px in range(pw):
            cell = px // scale
            f, x = divmod(cell, W + 1)
            if x == W:
                col = (60, 60, 60)
            else:
                p = rows[f * H + y][x]
                if p == T:
                    col = (150, 200, 120) if (x + y) % 2 else (130, 180, 100)
                else:
                    col = PALETTE[p]
            raw += bytes(col)
    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", pw, ph, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw)))
           + chunk(b"IEND", b""))
    Path(path).write_bytes(png)


def main(argv):
    out_dir = Path(argv[0]) if argv else Path(".")
    out_dir.mkdir(parents=True, exist_ok=True)
    rows = frames()
    write_bmp(out_dir / "pogo16x16.bmp", rows)
    (out_dir / "pogo16x16.json").write_text('{\n    "type": "sprite",\n    "height": 16\n}\n')
    if len(argv) > 1:
        write_preview(argv[1], rows)
    print(f"wrote {out_dir / 'pogo16x16.bmp'} and pogo16x16.json")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
