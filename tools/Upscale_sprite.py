#!/usr/bin/env python3
"""
Upscale_sprite.py

Writes a sprite twice as large: every pixel becomes a 2x2 block, palette and
bit depth stay, and the .json next to it gets the doubled frame height. Used
for the bosses, which are bigger versions of a world's traps.

    python3 tools/Upscale_sprite.py game/graphics/world1/kugelfisch16x16.bmp \\
        game/graphics/world1/kugelfisch32x32.bmp
"""

import json
import struct
import sys
from pathlib import Path


def bpp_of(path):
    return struct.unpack_from("<H", Path(path).read_bytes(), 28)[0]


def palette_of(path):
    data = Path(path).read_bytes()
    offset, = struct.unpack_from("<I", data, 10)
    header_size, = struct.unpack_from("<I", data, 14)
    return data[14 + header_size: offset]


def upscale(src, dst):
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "check_hitboxes", Path(__file__).with_name("Check_hitboxes.py"))
    hb = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(hb)
    rows = hb.read_bmp(src)
    big = []
    for row in rows:
        wide = [v for v in row for _ in (0, 1)]
        big += [wide, list(wide)]
    bpp = bpp_of(src)
    palette = palette_of(src).ljust(4 * (1 << bpp), b"\0")[: 4 * (1 << bpp)]
    hb.write_bmp(dst, big, bpp=bpp, palette=palette)
    meta = json.loads(Path(src).with_suffix(".json").read_text())
    if "height" in meta:
        meta["height"] *= 2
    if "width" in meta:
        meta["width"] *= 2
    Path(dst).with_suffix(".json").write_text(json.dumps(meta) + "\n")
    return Path(dst)


def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2
    print(f"wrote {upscale(argv[0], argv[1])}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
