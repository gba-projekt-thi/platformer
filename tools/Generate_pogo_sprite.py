#!/usr/bin/env python3
"""
Generate_pogo_sprite.py

Writes the three-frame pogo strike graphic (shown under the duck while the
down strike reaches, Player::_update_pogo_slash) as a Butano-ready 4bpp
indexed BMP plus its JSON:

    pogo16x16.bmp   16 x 48 px, three 16x16 frames stacked vertically
                    frame 0 = wide downward swoosh (a flattened V) with
                              speed streaks above it
                    frame 1 = the swoosh narrows and drops, impact star forms
                    frame 2 = impact star under the feet, swoosh fading
    pogo16x16.json  {"type": "sprite", "height": 16}

Palette index 0 is the transparent color (magenta), as Butano expects.
Pure Python (no Pillow). Usage:

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
OUT, WHITE, LIGHT, MID, YELLOW = 1, 2, 3, 4, 5

PALETTE = {
    T: (255, 0, 255),
    OUT: (34, 64, 140),
    WHITE: (255, 255, 255),
    LIGHT: (190, 240, 255),
    MID: (120, 190, 240),
    YELLOW: (255, 236, 39),
}


def blank():
    return [[T] * W for _ in range(H)]


def put(img, x, y, color):
    if 0 <= x < W and 0 <= y < H:
        img[y][x] = color


def outline(img):
    """Surround every drawn pixel with OUT where the neighbour is empty."""
    src = [row[:] for row in img]
    for y in range(H):
        for x in range(W):
            if src[y][x] != T:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < W and 0 <= ny < H and src[ny][nx] not in (T, OUT):
                    img[y][x] = OUT
                    break


def swoosh(img, tip_y, spread, colors):
    """A V pointing down: tip at the bottom centre, arms rising by `spread`
    rows over each half. Two pixels thick, bright on top."""
    for x in range(1, W - 1):  # leave a column for the outline
        d = abs(x - 7.5)  # 0.5 .. 6.5
        y = tip_y - int(round(d * spread))
        put(img, x, y, colors[0])
        put(img, x, y + 1, colors[1])


def streaks(img, ys):
    """Short vertical speed lines above the swoosh."""
    for x, y0, y1 in ys:
        for y in range(y0, y1 + 1):
            put(img, x, y, LIGHT)


def star(img, cx, cy, r):
    """Impact sparkle: plus shape with a bright core."""
    for i in range(-r, r + 1):
        put(img, cx + i, cy, YELLOW if abs(i) > 0 else WHITE)
        put(img, cx, cy + i, YELLOW if abs(i) > 0 else WHITE)
    if r >= 2:
        for dx, dy in ((1, 1), (-1, 1), (1, -1), (-1, -1)):
            put(img, cx + dx, cy + dy, WHITE)


def frame(index):
    img = blank()
    if index == 0:
        streaks(img, [(3, 1, 2), (12, 1, 2), (7, 0, 1)])
        swoosh(img, 10, 0.7, (WHITE, MID))
    elif index == 1:
        streaks(img, [(4, 3, 4), (11, 3, 4)])
        swoosh(img, 11, 0.5, (WHITE, MID))
        star(img, 7, 14, 1)
    else:
        swoosh(img, 8, 0.5, (LIGHT, MID))
        star(img, 7, 12, 2)
        put(img, 3, 13, YELLOW)
        put(img, 12, 13, YELLOW)
    outline(img)
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
