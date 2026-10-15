#!/usr/bin/env python3
"""
Generate_lever_sprite.py

Writes the two-frame lever/switch sprite used by Toggle triggers
(TriggerData::sprite) as a Butano-ready 4bpp indexed BMP plus its JSON:

    lever16x16.bmp   16 x 32 px, two 16x16 frames stacked vertically
                     frame 0 = off  (lever leans left,  red knob)
                     frame 1 = on   (lever leans right, green knob)
    lever16x16.json  {"type": "sprite", "height": 16}

Palette index 0 is the transparent color (magenta), as Butano expects.
Pure Python (no Pillow). Usage:

    python3 tools/Generate_lever_sprite.py game/graphics/global

The art is a placeholder-quality pixel drawing so switches are readable in
game; replace the BMP with hand-drawn art at any time - the name, size and
two-frame layout are all the game relies on.
"""

import struct
import sys
from pathlib import Path

W, H = 16, 16
T = 0  # transparent
OUT, STONE, LIGHT, METAL, RED, LRED, GREEN, LGREEN = 1, 2, 3, 4, 5, 6, 7, 8

PALETTE = {
    T: (255, 0, 255),
    OUT: (28, 24, 36),
    STONE: (104, 100, 118),
    LIGHT: (152, 148, 166),
    METAL: (196, 196, 208),
    RED: (206, 44, 44),
    LRED: (244, 120, 120),
    GREEN: (44, 184, 76),
    LGREEN: (148, 234, 156),
}


def blank():
    return [[T] * W for _ in range(H)]


def rect(img, x0, y0, x1, y1, color):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            img[y][x] = color


def line(img, x0, y0, x1, y1, color):
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        img[y0][x0] = color
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def frame(on):
    img = blank()
    # stone base with a dark slot
    rect(img, 2, 11, 13, 15, OUT)
    rect(img, 3, 12, 12, 14, STONE)
    rect(img, 3, 12, 12, 12, LIGHT)
    rect(img, 6, 11, 9, 12, OUT)
    # lever arm: outlined 2px stroke from the pivot to the tip
    tip_x = 12 if on else 3
    for off in (-1, 1):
        line(img, 8 + off, 11, tip_x + off, 4, OUT)
    line(img, 8, 11, tip_x, 4, METAL)
    # knob
    kx, ky = tip_x, 3
    body, shine = (GREEN, LGREEN) if on else (RED, LRED)
    rect(img, kx - 2, ky - 2, kx + 2, ky + 2, OUT)
    rect(img, kx - 1, ky - 1, kx + 1, ky + 1, body)
    img[ky - 1][kx - 1] = shine
    # re-draw the slot so the arm appears to come out of it
    rect(img, 7, 11, 8, 11, METAL)
    return img


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


def main(argv):
    out_dir = Path(argv[0]) if argv else Path(".")
    out_dir.mkdir(parents=True, exist_ok=True)
    rows = frame(False) + frame(True)
    write_bmp(out_dir / "lever16x16.bmp", rows)
    (out_dir / "lever16x16.json").write_text('{\n    "type": "sprite",\n    "height": 16\n}\n')
    print(f"wrote {out_dir / 'lever16x16.bmp'} and lever16x16.json")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
