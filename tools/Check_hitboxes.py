#!/usr/bin/env python3
"""
Check_hitboxes.py

Compares the collision box of every platform and trap in
game/include/level/levels_world*.h with the opaque pixels of the sprite frame
the game actually shows for it (a sprite is drawn centered on its (x, y); the
frame height comes from the sprite's .json, palette index 0 is transparent).

  - Platforms: the duck must stand ON the picture. Its feet are the lowest
    opaque row of ente.bmp, drawn Cfg::Player::SPRITE_OFFSET_Y above the body
    center; with the hitbox top below the picture's top the duck sinks in,
    above it the duck floats. A hitbox wider than the picture carries the duck
    in the air, a narrower one drops it off a visible edge. Each of the three
    may be off by at most PLATFORM_TOLERANCE px.
  - Traps: a trap must not kill before the pictures touch. Its hitbox may
    reach past its picture only as far as the duck's own picture reaches past
    the duck's body on the facing side, plus TRAP_TOLERANCE px.
  - Doors: the box (door_logic.h) lies inside the opaque pixels of every frame
    the door shows, at most DOOR_MAX_INSET px in from each side, so the duck
    has to visibly walk into the door but cannot be missed by a wide margin.

Usage (from the repository root):
    python3 tools/Check_hitboxes.py

Exit code 0 = every box matches its picture, 1 = problems found.
"""

import importlib.util
import json
import re
import struct
import sys
from pathlib import Path

_here = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location(
    "check_trigger_references", _here / "Check_trigger_references.py"
)
_ctr = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_ctr)

PLATFORM_TOLERANCE = 3
TRAP_TOLERANCE = 2
DOOR_MAX_INSET = 4  # px the door box may sit inside its picture, per side
DOOR_MAX_OUTSET = 0  # px it may reach past it


# ---- bitmaps ----------------------------------------------------------------
def read_bmp(path):
    """Rows (top to bottom) of palette indexes of an uncompressed 4/8 bpp BMP."""
    data = Path(path).read_bytes()
    offset, = struct.unpack_from("<I", data, 10)
    width, height = struct.unpack_from("<ii", data, 18)
    bpp, = struct.unpack_from("<H", data, 28)
    if bpp not in (4, 8):
        raise ValueError(f"{path}: {bpp} bpp is not supported")
    stride = ((width * bpp + 31) // 32) * 4
    rows = []
    for r in range(abs(height)):
        src = abs(height) - 1 - r if height > 0 else r
        raw = data[offset + src * stride: offset + (src + 1) * stride]
        if bpp == 8:
            rows.append(list(raw[:width]))
        else:
            pixels = []
            for byte in raw[: (width + 1) // 2]:
                pixels += [byte >> 4, byte & 15]
            rows.append(pixels[:width])
    return rows


def write_bmp(path, rows, bpp=8, palette=None):
    """Writes rows of palette indexes as a bottom-up 4/8 bpp BMP. `palette` is
    the raw BGRA palette bytes (default: grey ramp)."""
    width, height = len(rows[0]), len(rows)
    colors = 1 << bpp
    if palette is None:
        palette = b"".join(bytes((i, i, i, 0)) for i in range(0, 256, 256 // colors))
    stride = ((width * bpp + 31) // 32) * 4
    pixels = b""
    for row in reversed(rows):
        if bpp == 8:
            raw = bytes(row)
        else:
            padded = row + [0] * (len(row) % 2)
            raw = bytes((padded[i] << 4) | padded[i + 1] for i in range(0, len(padded), 2))
        pixels += raw + b"\0" * (stride - len(raw))
    offset = 14 + 40 + len(palette)
    header = struct.pack("<2sIHHI", b"BM", offset + len(pixels), 0, 0, offset)
    info = struct.pack(
        "<IiiHHIIiiII", 40, width, height, 1, bpp, 0, len(pixels), 2835, 2835, colors, 0
    )
    Path(path).write_bytes(header + info + palette + pixels)


def frame_box(rows, frame_height, index):
    """Opaque box (left, top, right, bottom) of frame `index` (wrapping like
    sprite_index % graphics_count), relative to the frame center; None if the
    frame is empty."""
    width = len(rows[0])
    index %= len(rows) // frame_height
    frame = rows[index * frame_height: (index + 1) * frame_height]
    ys = [y for y, row in enumerate(frame) if any(row)]
    xs = [x for x in range(width) if any(row[x] for row in frame)]
    if not ys:
        return None
    return (xs[0] - width / 2, ys[0] - frame_height / 2,
            xs[-1] + 1 - width / 2, ys[-1] + 1 - frame_height / 2)


class Sprites:
    """Sprite sheets under game/graphics, by Butano item name."""

    def __init__(self, root):
        self._paths = {p.stem: p for p in (Path(root) / "game/graphics").rglob("*.bmp")}
        self._sheets = {}

    def __contains__(self, name):
        return name in self._paths

    def box(self, name, index):
        if name not in self._sheets:
            path = self._paths[name]
            rows = read_bmp(path)
            height = json.loads(path.with_suffix(".json").read_text()).get("height", len(rows))
            self._sheets[name] = (rows, height)
        rows, height = self._sheets[name]
        return frame_box(rows, height, index)

    def frame_count(self, name):
        self.box(name, 0)
        rows, height = self._sheets[name]
        return len(rows) // height


# ---- rules ------------------------------------------------------------------
def platform_mismatch(hit, img, feet=0):
    """(sink, left, right) in px for world boxes (left, top, right, bottom).
    sink > 0: the duck's feet are inside the picture, < 0: above it. left /
    right > 0: the hitbox reaches past the picture (the duck stands in the
    air), < 0: the picture reaches past the hitbox (the duck drops early).
    `feet`: how far the duck's picture reaches below its body."""
    return (hit[1] + feet - img[1], img[0] - hit[0], hit[2] - img[2])


def trap_unfair_by(hit, img, duck):
    """Px by which the trap's hitbox reaches past its picture further than the
    duck's picture (margins left, top, right, bottom past its body) can cover
    on the facing side; 0 = fair."""
    margin_left, margin_top, margin_right, margin_bottom = duck
    return max(
        0,
        img[0] - hit[0] - margin_right,   # trap's left side meets the duck's right
        img[1] - hit[1] - margin_bottom,  # trap's top meets the duck's feet
        hit[2] - img[2] - margin_left,
        hit[3] - img[3] - margin_top,
    )


def door_problem(hit, img):
    """Why a door box (left, top, right, bottom; relative to the sprite's
    center) does not fit the opaque pixels `img` of its picture, else None."""
    for side, inset in zip(("left", "top", "right", "bottom"),
                           (hit[0] - img[0], hit[1] - img[1], img[2] - hit[2], img[3] - hit[3])):
        if inset < -DOOR_MAX_OUTSET:
            return f"hitbox reaches {-inset:g} px past the picture on the {side}"
        if inset > DOOR_MAX_INSET:
            return f"hitbox is {inset:g} px inside the picture on the {side}"
    return None


# ---- game data --------------------------------------------------------------
def cfg_player(root, constant):
    text = (Path(root) / "game/include/cfg.h").read_text()
    block = re.search(r"namespace Player \{(.*?)\}  // namespace Player", text, re.DOTALL)
    match = re.search(r"\b" + constant + r"\s*=\s*(-?\d+)", block.group(1))
    if match is None:
        raise ValueError(f"Cfg::Player::{constant} not found in cfg.h")
    return int(match.group(1))


def duck_margins(root):
    """How far the duck's picture (every frame of ente.bmp, shifted by
    Cfg::Player::SPRITE_OFFSET_Y) reaches past its body: (left, top, right,
    bottom)."""
    sprites = Sprites(root)
    boxes = [sprites.box("ente", i) for i in range(sprites.frame_count("ente"))]
    boxes = [b for b in boxes if b]
    offset = cfg_player(root, "SPRITE_OFFSET_Y")
    half_w = cfg_player(root, "BODY_WIDTH") / 2
    half_h = cfg_player(root, "BODY_HEIGHT") / 2
    return (
        -half_w - min(b[0] for b in boxes),
        -half_h - (min(b[1] for b in boxes) + offset),
        max(b[2] for b in boxes) - half_w,
        max(b[3] for b in boxes) + offset - half_h,
    )


def _num(text):
    return float(text.strip())


def _sprite(field):
    match = re.search(r"sprite_items::(\w+)", field)
    return match.group(1) if match else None


def _index_arrays(texts):
    """constexpr uint16_t NAME[] = {...} animation sequences."""
    arrays = {}
    for text in texts:
        for name, body in re.findall(r"uint16_t\s+(\w+)\[\]\s*=\s*\{([^}]*)\}", text):
            arrays[name] = [int(v) for v in re.findall(r"\d+", body)]
    return arrays


def _union(boxes):
    boxes = [b for b in boxes if b]
    if not boxes:
        return None
    return (min(b[0] for b in boxes), min(b[1] for b in boxes),
            max(b[2] for b in boxes), max(b[3] for b in boxes))


def scaled_box(box, scale):
    """`box` (relative to the sprite's center) drawn `scale` times larger."""
    return tuple(v * scale for v in box)


def trap_field_index(field):
    """Position of a TrapData field in the generated (positional) literals."""
    spec = importlib.util.spec_from_file_location(
        "generate_levels", Path(__file__).with_name("Generate_levels.py"))
    gen = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gen)
    return [name for name, _ in gen.read_structs()["TrapData"]].index(field)


def door_boxes(root):
    """{sprite height: (left, top, right, bottom)} of the DOOR_BOX_* constants
    in game/include/door_logic.h (width, height, offset_x, offset_y)."""
    text = (Path(root) / "game/include/door_logic.h").read_text()
    boxes = {}
    for size, w, h, ox, oy in re.findall(
            r"DOOR_BOX_(\d+)\s*=\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}", text):
        w, h, ox, oy = int(w), int(h), int(ox), int(oy)
        boxes[int(size)] = (ox - w / 2, oy - h / 2, ox + w / 2, oy + h / 2)
    return boxes


def _world(box, x, y):
    return (x + box[0], y + box[1], x + box[2], y + box[3])


def check(root):
    root = Path(root)
    sprites = Sprites(root)
    duck = duck_margins(root)
    level_dir = root / "game/include/level"
    files = sorted(level_dir.glob("levels_world*.h"))
    texts = {path: _ctr.strip_comments(path.read_text()) for path in files}
    sequences = _index_arrays([(level_dir / "levels_common.h").read_text(), *texts.values()])
    problems = []
    scale_at = trap_field_index("sprite_scale")
    boxes = door_boxes(root)

    for path, text in texts.items():
        for array, body in _ctr.extract_named_arrays(text, "PlatformData").items():
            for i, literal in enumerate(_ctr.extract_top_level_literals(body)):
                f = _ctr.top_level_fields(literal)
                x, y, w, h, ox, oy = (_num(v) for v in f[:6])
                name = _sprite(f[6])
                where = f"{path.name} {array}[{i}] {name}"
                if name not in sprites:
                    problems.append(f"{where}: unknown sprite")
                    continue
                img = sprites.box(name, int(_num(f[7])))
                hit = (x + ox - w / 2, y + oy - h / 2, x + ox + w / 2, y + oy + h / 2)
                sink, left, right = platform_mismatch(hit, _world(img, x, y), duck[3])
                if abs(sink) > PLATFORM_TOLERANCE:
                    what = "sinks into" if sink > 0 else "floats above"
                    problems.append(f"{where}: the duck {what} the picture by {abs(sink):g} px")
                for side, delta in (("left", left), ("right", right)):
                    if abs(delta) > PLATFORM_TOLERANCE:
                        what = "past" if delta > 0 else "short of"
                        problems.append(
                            f"{where}: hitbox ends {abs(delta):g} px {what} the picture on the {side}"
                        )

        for array, body in _ctr.extract_named_arrays(text, "TrapData").items():
            for i, literal in enumerate(_ctr.extract_top_level_literals(body)):
                f = _ctr.top_level_fields(literal)
                x, y, w, h, ox, oy = (_num(v) for v in f[1:7])
                name = _sprite(f[7])
                where = f"{path.name} {array}[{i}] {name}"
                if name not in sprites:
                    problems.append(f"{where}: unknown sprite")
                    continue
                frames = sequences.get(f[9].strip(), [0]) if len(f) > 9 else [0]
                img = _union([sprites.box(name, k) for k in frames])
                if len(f) > scale_at and img:
                    img = scaled_box(img, _num(f[scale_at]))
                hit = (x + ox - w / 2, y + oy - h / 2, x + ox + w / 2, y + oy + h / 2)
                unfair = trap_unfair_by(hit, _world(img, x, y), duck)
                if unfair > TRAP_TOLERANCE:
                    problems.append(f"{where}: kills {unfair:g} px before the pictures touch")

        for m in re.finditer(r"(?:const|constexpr) LevelData (\w+)\s*=\s*\{", text):
            end = _ctr.find_balanced_block(text, m.end() - 1)
            level = _ctr.top_level_fields(text[m.end():end])
            door = _ctr.top_level_fields(level[9].strip()[1:-1])
            name = _sprite(door[2]) if len(door) > 2 else "door32x32"
            where = f"{path.name} {m.group(1)} door {name}"
            if name not in sprites:
                problems.append(f"{where}: unknown sprite")
                continue
            frames = [0, 2, 4, 6, 8, 10, 12]  # DOOR32X32_GRAPHICS_INDEXES
            if len(door) > 3:
                frames = sequences.get(door[3].strip(), [0])
            img = _union([sprites.box(name, k) for k in frames])
            height = json.loads(
                sprites._paths[name].with_suffix(".json").read_text()).get("height", 32)
            if height not in boxes:
                problems.append(f"{where}: no DOOR_BOX_{height} in door_logic.h")
                continue
            reason = door_problem(boxes[height], img)
            if reason:
                problems.append(f"{where}: {reason}")
    return problems


def main(argv, root="."):
    problems = check(root)
    for problem in problems:
        print(f"ERROR {problem}", file=sys.stderr)
    print(f"\nHitbox check: {len(problems)} problem(s).")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
