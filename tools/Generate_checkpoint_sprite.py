#!/usr/bin/env python3
"""
Generate_checkpoint_sprite.py

Writes the two-frame checkpoint flag (Checkpoint, LevelData::checkpoints) as a
Butano-ready 4bpp indexed BMP plus its JSON:

    checkpoint16x16.bmp   16 x 32 px, two 16x16 frames stacked vertically
                          frame 0 = not reached (grey flag hanging low)
                          frame 1 = active      (green flag raised)
    checkpoint16x16.json  {"type": "sprite", "height": 16}

It reuses the palette and the pixel helpers of Generate_lever_sprite.py, so the
flag matches the lever art. Palette index 0 is the transparent color (magenta).
Pure Python (no Pillow). Usage:

    python3 tools/Generate_checkpoint_sprite.py game/graphics/global

The art is placeholder-quality pixel drawing; replace the BMP with hand-drawn
art at any time - the name, size and two-frame layout are all the game relies
on.
"""

import importlib.util
import sys
from pathlib import Path

_spec = importlib.util.spec_from_file_location(
    "generate_lever_sprite", Path(__file__).with_name("Generate_lever_sprite.py")
)
lever = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(lever)

OUT, STONE, LIGHT, METAL = lever.OUT, lever.STONE, lever.LIGHT, lever.METAL
GREEN, LGREEN = lever.GREEN, lever.LGREEN


def frame(active):
    img = lever.blank()
    # stone foot
    lever.rect(img, 3, 13, 12, 15, OUT)
    lever.rect(img, 4, 14, 11, 14, STONE)
    lever.rect(img, 4, 14, 11, 14, STONE)
    # pole with a small knob
    lever.rect(img, 6, 1, 9, 13, OUT)
    lever.rect(img, 7, 2, 8, 12, METAL)
    img[2][7] = LIGHT
    if active:
        # raised green flag at the top of the pole
        lever.rect(img, 9, 2, 14, 7, OUT)
        lever.rect(img, 10, 3, 13, 6, GREEN)
        img[3][10] = LGREEN
        img[3][11] = LGREEN
    else:
        # grey flag hanging low
        lever.rect(img, 9, 8, 13, 11, OUT)
        lever.rect(img, 10, 9, 12, 10, STONE)
    return img


def main(argv):
    out_dir = Path(argv[0]) if argv else Path(".")
    out_dir.mkdir(parents=True, exist_ok=True)
    rows = frame(False) + frame(True)
    lever.write_bmp(out_dir / "checkpoint16x16.bmp", rows)
    (out_dir / "checkpoint16x16.json").write_text(
        '{\n    "type": "sprite",\n    "height": 16\n}\n'
    )
    print(f"wrote {out_dir / 'checkpoint16x16.bmp'} and checkpoint16x16.json")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
