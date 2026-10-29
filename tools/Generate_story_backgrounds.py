#!/usr/bin/env python3
"""
Generate_story_backgrounds.py

Writes the six picture backgrounds of the story pages (story.h, shown by
StoryScene) as Butano-ready 4bpp indexed BMPs plus their JSONs:

    story_intro.bmp    the bathroom tiles, a dangling plug chain, the drain
    story_sewer.bmp    brick wall, a rusty pipe, murky water
    story_forest.bmp   teal tree trunks, fog, mushrooms
    story_village.bmp  sky, half-timbered roofs, grass and flowers
    story_castle.bmp   a castle against the dusk, cobblestones
    story_ending.bmp  sunset, Hildegard and Susanne, hearts

Each is a 240 x 160 px picture (one GBA screen) in the top-left corner of a
256 x 256 px BMP (Butano's size rule), with at most 16 colors; palette index
0 is black (the backdrop). The middle of every picture is a dark panel with
a light border where StoryScene puts its text (PANEL_* below match
Cfg::Story in game/include/cfg.h: the text stays readable on any scenery);
the scenery lives around it. Hildegard's colors follow ente.bmp, Susanne's
follow susanne16x16.bmp. Pure Python (no Pillow). Usage:

    python3 tools/Generate_story_backgrounds.py game/graphics/story [preview_dir]

The optional second argument also writes an enlarged PNG preview per picture.
"""

import struct
import sys
import zlib
from pathlib import Path

W, H = 240, 160
# The text panel (screen pixels, inclusive). Cfg::Story: lines from y = 46 to
# 92 and the prompt at 110 (screen coordinates) lie inside.
PANEL = (18, 28, 221, 122)

BLACK = (0, 0, 0)
PANEL_FILL = (22, 26, 52)
PANEL_BORDER = (194, 195, 199)
YELLOW = (255, 236, 39)
CREAM = (255, 241, 232)
ORANGE = (255, 163, 0)
PINK = (255, 119, 168)
PINK_DARK = (171, 82, 120)
RED = (255, 0, 77)

DUCK = (
    "......kkk.....",
    ".....kyyyk....",
    ".....kykyk....",
    ".....kyyykkk..",
    ".....kyyyoook.",
    ".kk..kyyyykk..",
    "kyykkyyyyyk...",
    "kyyyyyyyyyk...",
    ".kyyccyyyyk...",
    "..kyyyyyyk....",
    "...kkkkkk.....",
)
CROWN = (
    "g.g.g",
    "ggggg",
)
HEART = (
    ".rr.rr.",
    "rrrrrrr",
    "rrrrrrr",
    ".rrrrr.",
    "..rrr..",
    "...r...",
)


class Canvas:
    def __init__(self):
        self.px = [[BLACK] * W for _ in range(H)]

    def dot(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            self.px[y][x] = c

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.dot(x, y, c)

    def hline(self, x0, x1, y, c):
        self.rect(x0, y, x1, y, c)

    def ellipse(self, cx, cy, rx, ry, c, ring=False):
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
                if (d <= 1.0 and not ring) or (ring and 0.82 <= d <= 1.0):
                    self.dot(x, y, c)

    def bitmap(self, x, y, rows, colors, mirror=False, scale=1):
        for dy, row in enumerate(rows):
            if mirror:
                row = row[::-1]
            for dx, ch in enumerate(row):
                if ch in colors:
                    self.rect(x + dx * scale, y + dy * scale,
                              x + dx * scale + scale - 1,
                              y + dy * scale + scale - 1, colors[ch])

    def duck(self, x, y, mirror=False):
        self.bitmap(x, y, DUCK, {"k": BLACK, "y": YELLOW, "c": CREAM, "o": ORANGE}, mirror, 2)

    def susanne(self, x, y, mirror=False):
        colors = {"k": BLACK, "y": PINK, "c": PINK_DARK, "o": ORANGE}
        self.bitmap(x, y, DUCK, colors, mirror, 2)
        self.bitmap(x + (8 if mirror else 10), y - 4, CROWN, {"g": YELLOW}, False, 2)

    def heart(self, x, y):
        self.bitmap(x, y, HEART, {"r": RED})

    def panel(self):
        x0, y0, x1, y1 = PANEL
        self.rect(x0, y0, x1, y1, PANEL_FILL)
        self.rect(x0, y0, x1, y0, PANEL_BORDER)
        self.rect(x0, y1, x1, y1, PANEL_BORDER)
        self.rect(x0, y0, x0, y1, PANEL_BORDER)
        self.rect(x1, y0, x1, y1, PANEL_BORDER)


# ---------------------------------------------------------------- scenes ----

def intro(c):
    tile, grout, water, deep, foam = (222, 236, 240), (150, 180, 190), (41, 173, 255), (30, 110, 190), (255, 255, 255)
    c.rect(0, 0, W - 1, 110, tile)
    for y in range(0, 111, 16):
        c.hline(0, W - 1, y, grout)
    for row, y in enumerate(range(0, 111, 16)):
        for x in range(8 * (row % 2), W, 16):
            c.rect(x, y, x, y + 15, grout)
    c.rect(0, 100, W - 1, 110, (190, 210, 220))
    # The plug on its chain, pulled out of the water.
    for y in range(0, 22, 3):
        c.dot(205, y, (130, 130, 140))
        c.dot(206, y + 1, (130, 130, 140))
    c.ellipse(205, 25, 6, 4, (120, 30, 40))
    c.ellipse(205, 24, 3, 1, (200, 70, 80))
    # Water with a whirlpool toward the drain.
    c.rect(0, 111, W - 1, H - 1, water)
    for i, (rx, ry) in enumerate(((115, 17), (92, 14), (68, 11), (46, 8), (26, 5))):
        c.ellipse(120, 148, rx, ry, deep if i % 2 == 0 else foam, ring=True)
    for x in range(6, W, 24):
        c.hline(x, x + 7, 118, foam)
        c.hline(x + 10, x + 15, 126, deep)
    c.duck(20, 128)


def sewer(c):
    brick, mortar, dark, pipe, rust, drop = (122, 48, 36), (170, 130, 110), (70, 28, 24), (110, 100, 96), (180, 100, 40), (90, 140, 100)
    c.rect(0, 0, W - 1, H - 1, brick)
    for y in range(0, 135, 10):
        c.hline(0, W - 1, y, mortar)
        for x in range(5 * ((y // 10) % 2) * 2, W, 20):
            c.rect(x, y, x, y + 9, mortar)
    # A rusty pipe across the top, rivets, a drip.
    c.rect(0, 5, W - 1, 17, pipe)
    c.hline(0, W - 1, 5, rust)
    c.hline(0, W - 1, 17, dark)
    for x in range(10, W, 30):
        c.rect(x, 5, x + 2, 17, rust)
        c.dot(x + 1, 8, dark)
        c.dot(x + 1, 14, dark)
    c.rect(200, 18, 201, 22, drop)
    c.rect(200, 26, 201, 27, drop)
    # Murky water.
    c.rect(0, 132, W - 1, H - 1, (50, 90, 70))
    for x in range(4, W, 28):
        c.hline(x, x + 9, 138, (90, 140, 100))
        c.hline(x + 12, x + 19, 147, dark)
    c.rect(0, 132, W - 1, 133, dark)
    c.duck(30, 128)
    # A can floating by.
    c.rect(190, 134, 208, 150, pipe)
    c.rect(190, 134, 208, 137, mortar)
    c.rect(190, 143, 208, 145, (200, 60, 60))


def forest(c):
    sky, trunk_a, trunk_b, bark, fog, ground, cap, white = (22, 44, 52), (38, 78, 88), (28, 60, 70), (14, 30, 36), (130, 190, 195), (18, 36, 40), (255, 0, 77), (255, 241, 232)
    c.rect(0, 0, W - 1, H - 1, sky)
    for i, x in enumerate(range(6, W, 30)):
        w = 12 + (i * 7) % 9
        c.rect(x, 0, x + w, 140, trunk_a if i % 2 else trunk_b)
        c.rect(x, 0, x + 1, 140, bark)
        for y in range(10 + (i * 11) % 17, 140, 24):
            c.hline(x + 3, x + 8, y, bark)
    c.rect(0, 0, W - 1, 12, (12, 26, 30))
    for x in range(0, W, 9):
        c.ellipse(x, 12, 6, 5, (12, 26, 30))
    # Fog and the forest floor.
    for y in range(118, 145, 4):
        c.hline(0, W - 1, y, fog)
    c.rect(0, 140, W - 1, H - 1, ground)
    for x in (22, 190, 214):
        c.rect(x + 3, 143, x + 5, 149, white)
        c.ellipse(x + 4, 143, 6, 4, cap)
        c.dot(x + 2, 142, white)
        c.dot(x + 6, 141, white)
    c.duck(60, 136)


def village(c):
    sky, cloud, wall, beam, roof, roof_dark, grass, grass_dark, hill, f1, f2 = (
        (130, 200, 255), (255, 255, 255), (255, 241, 232), (90, 52, 40), (190, 90, 70),
        (130, 55, 50), (80, 170, 70), (50, 120, 55), (110, 190, 130), (255, 236, 39), (255, 119, 168))
    c.rect(0, 0, W - 1, H - 1, sky)
    for cx, cy in ((40, 12), (170, 8), (215, 20)):
        c.ellipse(cx, cy, 14, 5, cloud)
        c.ellipse(cx + 8, cy - 3, 8, 4, cloud)
    c.ellipse(60, 135, 120, 40, hill)
    c.ellipse(200, 140, 100, 30, hill)
    # Half-timbered houses: the gables rise into the top band.
    for x, w, top in ((4, 46, 20), (66, 56, 14), (140, 50, 18), (194, 42, 22)):
        mid = x + w // 2
        for row in range(24):
            half = min(w // 2, 4 + row * w // 48)
            c.hline(mid - half, mid + half, top + row, roof if row % 4 else roof_dark)
        c.rect(x + 3, top + 24, x + w - 3, 130, wall)
        c.rect(x + 3, top + 24, x + 4, 130, beam)
        c.rect(x + w - 4, top + 24, x + w - 3, 130, beam)
        c.hline(x + 3, x + w - 3, top + 24, beam)
        c.hline(x + 3, x + w - 3, top + 40, beam)
    c.rect(0, 132, W - 1, H - 1, grass)
    for x in range(0, W, 6):
        c.dot(x, 134 + (x * 5) % 7, grass_dark)
        c.dot(x + 2, 142 + (x * 3) % 9, grass_dark)
    for i, x in enumerate((8, 46, 150, 176, 228)):
        c.rect(x, 144, x, 150, grass_dark)
        c.dot(x, 143, f1 if i % 2 else f2)
        c.dot(x - 1, 144, f1 if i % 2 else f2)
        c.dot(x + 1, 144, f1 if i % 2 else f2)
    c.duck(190, 130, mirror=True)


def castle(c):
    sky_a, sky_b, sky_c, stone, stone_dark, win, floor, floor_line, flag = (
        (60, 40, 90), (110, 60, 110), (170, 90, 120), (60, 56, 76), (38, 34, 52), (255, 200, 60),
        (74, 70, 88), (40, 38, 54), (255, 0, 77))
    c.rect(0, 0, W - 1, 12, sky_a)
    c.rect(0, 13, W - 1, 24, sky_b)
    c.rect(0, 25, W - 1, H - 1, sky_c)
    # The castle: towers with battlements against the dusk.
    for x0, x1, top in ((16, 52, 8), (74, 166, 14), (188, 224, 4)):
        c.rect(x0, top, x1, 135, stone)
        for x in range(x0, x1 - 2, 8):
            c.rect(x, top - 5, x + 4, top, stone)
        for x in range(x0 + 6, x1 - 4, 14):
            c.rect(x, top + 8, x + 3, top + 16, win)
        c.rect(x0, top, x0 + 1, 135, stone_dark)
    c.rect(33, 0, 34, 4, stone_dark)
    c.rect(35, 0, 41, 3, flag)
    c.rect(205, 0, 206, 0, stone_dark)
    # Cobblestones.
    c.rect(0, 134, W - 1, H - 1, floor)
    for row, y in enumerate(range(134, H, 8)):
        c.hline(0, W - 1, y, floor_line)
        for x in range(6 * (row % 2), W, 12):
            c.rect(x, y, x, y + 7, floor_line)
    c.duck(100, 130)


def ending(c):
    sky = [(255, 204, 170), (255, 163, 130), (240, 120, 100), (200, 80, 110)]
    for i, col in enumerate(sky):
        c.rect(0, i * 10, W - 1, i * 10 + 9, col)
    c.rect(0, 40, W - 1, H - 1, (255, 150, 110))
    c.ellipse(120, 70, 28, 28, (255, 236, 170))
    c.rect(0, 138, W - 1, H - 1, (60, 110, 80))
    c.ellipse(60, 146, 90, 14, (60, 110, 80))
    c.ellipse(190, 150, 90, 12, (50, 90, 70))
    c.rect(0, 150, W - 1, H - 1, (50, 90, 70))
    for x, y in ((22, 6), (60, 14), (104, 4), (150, 12), (196, 5), (222, 14)):
        c.heart(x, y)
    c.duck(84, 128)
    c.susanne(144, 128, mirror=True)
    c.heart(121, 128)


SCENES = (
    ("story_intro", intro),
    ("story_sewer", sewer),
    ("story_forest", forest),
    ("story_village", village),
    ("story_castle", castle),
    ("story_ending", ending),
)


def render(draw):
    c = Canvas()
    draw(c)
    c.panel()
    return c.px


def quantize(px):
    palette = [BLACK]
    for row in px:
        for col in row:
            if col not in palette:
                palette.append(col)
    assert len(palette) <= 16, f"{len(palette)} colors"
    return [[palette.index(col) for col in row] for row in px], palette


def write_bmp(path, rows, palette):
    # Butano wants a regular background in multiples of 256: the picture sits
    # in the top-left corner (the screen), the rest is black.
    rows = [row + [0] * (256 - W) for row in rows] + [[0] * 256 for _ in range(256 - H)]
    height, width = len(rows), len(rows[0])
    row_bytes = ((width * 4 + 31) // 32) * 4
    data = bytearray()
    for y in range(height - 1, -1, -1):  # BMP is bottom-up
        row = bytearray(row_bytes)
        for x in range(0, width, 2):
            row[x // 2] = (rows[y][x] << 4) | rows[y][x + 1]
        data += row
    pal = bytearray()
    for i in range(16):
        r, g, b = palette[i] if i < len(palette) else (0, 0, 0)
        pal += bytes((b, g, r, 0))
    offset = 14 + 40 + len(pal)
    header = b"BM" + struct.pack("<IHHI", offset + len(data), 0, 0, offset)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 4, 0, len(data), 2835, 2835, 16, 16)
    Path(path).write_bytes(header + info + bytes(pal) + bytes(data))


def write_png(path, px, scale=3):
    raw = bytearray()
    for row in px:
        line = bytearray([0])
        for col in row:
            line += bytes(col) * scale
        for _ in range(scale):
            raw += line

    def chunk(tag, body):
        b = tag + body
        return struct.pack(">I", len(body)) + b + struct.pack(">I", zlib.crc32(b))

    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", W * scale, H * scale, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw)))
           + chunk(b"IEND", b""))
    Path(path).write_bytes(png)


def main(argv):
    out = Path(argv[0]) if argv else Path(".")
    out.mkdir(parents=True, exist_ok=True)
    for name, draw in SCENES:
        px = render(draw)
        rows, palette = quantize(px)
        write_bmp(out / f"{name}.bmp", rows, palette)
        (out / f"{name}.json").write_text('{\n    "type": "regular_bg"\n}\n')
        if len(argv) > 1:
            Path(argv[1]).mkdir(parents=True, exist_ok=True)
            write_png(Path(argv[1]) / f"{name}.png", px)
        print(f"wrote {out / name}.bmp ({len(palette)} colors)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
