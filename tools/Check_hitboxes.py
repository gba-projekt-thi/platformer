#!/usr/bin/env python3
"""
Check_hitboxes.py

Compares every collision box in the game with the opaque pixels of the sprite
frames it belongs to (a sprite is drawn centered on its (x, y); the frame
height comes from the sprite's .json, palette index 0 is transparent). Every
animation frame is judged on its own. Boxes are in px, "past" means outside.

  - Platforms: the duck must stand ON the picture. Its feet are the lowest
    opaque row of ente.bmp, drawn Cfg::Player::SPRITE_OFFSET_Y above the body
    center; with the hitbox top below the picture's top the duck sinks in,
    above it the duck floats. A hitbox wider than the picture carries the duck
    in the air, a narrower one drops it off a visible edge. Each of the three
    may be off by at most PLATFORM_TOLERANCE px. This covers every
    PlatformData: ordinary, crumbling, switched, fleeing, per-death ones.
  - Everything that hurts or reacts to the duck (every TrapData of every type
    - BASE, MOVING, PATH, CHASE, AMBUSH, CYCLE, BOSS, fake doors - plus the
    visible levers/plates of TriggerData and the checkpoint flags) is judged
    frame by frame over all frames the game shows (the graphics_indexes
    sequence, wrapped like the game; scaled by sprite_scale):
      1. A frame must never show less than the box: in no frame may the box
         reach past that frame's picture by more than the duck's own picture
         reaches past its body on the facing side, plus TRAP_TOLERANCE px.
         An empty frame under a live box is a problem. (No kill on pixels
         that are not drawn at that moment.)
      2. The box must not be much smaller than the picture: what EVERY frame
         draws (the intersection of all frames) may reach past the box by at
         most TRAP_UNDER_TOLERANCE px per side. (No visibly overlapping
         pictures without a kill.) Parts drawn in only some frames of an
         animation do not count for this.
    A BASE trap with a door sprite is a fake door; it must also match the real
    door box of door_logic.h within FAKE_DOOR_TOLERANCE px.
  - Doors: the box (door_logic.h) lies inside the opaque pixels of every frame
    the door shows (each frame on its own), at most DOOR_MAX_INSET px in from
    each side, so the duck has to visibly walk into the door but cannot be
    missed by a wide margin.
  - The duck (ente.bmp, every frame incl. run, jump, idle, back, wall slide;
    each also mirrored): its 8x8 body (Cfg::Player) must not reach past the
    frame's picture by more than DUCK_BODY_TOLERANCE px, its feet must rest on
    the body's bottom within DUCK_FEET_TOLERANCE px, and the picture may reach
    past the body by at most DUCK_MAX_SIDE_MARGIN px sideways and
    DUCK_MAX_HEAD_MARGIN above (the head is deliberately not hurtable).
  - The pogo strike (Cfg::Pogo, pogo16x16.bmp): its box lies under the duck's
    body, the picture is centered on it; same rules 1 and 2 as for traps but
    without the duck margin.
  - Checkpoints (checkpoint.cpp touch zone vs checkpoint16x16.bmp): rules 1 and
    2 above, with the duck margin.

Coverage: every sprite sheet under game/graphics (type "sprite") must either
be checked by one of the rules above (it is used by a platform, trap, trigger,
door, the duck, the pogo strike or a checkpoint) or be listed in NO_BODY with
the reason it has no collision body. A stale NO_BODY entry (sprite checked
anyway, or gone) fails too, so a new sprite with a body cannot slip through.

Usage (from the repository root):
    python3 tools/Check_hitboxes.py            # problems only
    python3 tools/Check_hitboxes.py --report   # also the numbers of every
                                               # frame of every box

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
TRAP_UNDER_TOLERANCE = 4  # px the always-drawn picture may reach past the box
FAKE_DOOR_TOLERANCE = 2  # px a fake door's box may differ from the real one
DOOR_SPRITES = {"door32x32"}  # a BASE trap with one of these is a fake door
DUCK_BODY_TOLERANCE = 1  # px the body may reach past the duck's picture
DUCK_FEET_TOLERANCE = 1  # px the feet may be off the body's bottom
DUCK_MAX_SIDE_MARGIN = 4  # px the picture may reach past the body sideways
DUCK_MAX_HEAD_MARGIN = 8  # px the picture may reach above the body

# Sprite sheets without a collision body, and why. Everything else under
# game/graphics must be checked above.
NO_BODY = {
    "titel64x128": "title screen logo (start_scene.cpp)",
    "schnabel64x64": "title screen decoration (start_scene.cpp)",
    "tail64x64": "title screen decoration (start_scene.cpp, currently commented out)",
    "bubbles32x32": "not used by any level or code",
    "kugelfisch": "not used by any level or code",
    "ladder": "not used by any level or code",
}


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


SIDES = ("left", "top", "right", "bottom")


def _intersection(boxes):
    """Box every one of `boxes` covers, None if they do not overlap."""
    box = (max(b[0] for b in boxes), max(b[1] for b in boxes),
           min(b[2] for b in boxes), min(b[3] for b in boxes))
    return box if box[0] < box[2] and box[1] < box[3] else None


def frame_numbers(hit, frames, margins):
    """{frame: px by which the box kills before the pictures touch}."""
    return {k: (trap_unfair_by(hit, img, margins) if img else None)
            for k, img in frames.items()}


def box_problems(hit, frames, margins, over_tol=TRAP_TOLERANCE, under_tol=TRAP_UNDER_TOLERANCE):
    """Why a hit box does not fit the pictures of its frames, else [].
    `frames`: {frame index: opaque box or None}, same coordinates as `hit`;
    `margins`: how far the toucher's own picture reaches past its body
    (left, top, right, bottom). Rule 1 (per frame): no kill before the
    pictures touch. Rule 2: what every frame draws may reach past the box by
    at most under_tol px per side."""
    problems = []
    for k, img in sorted(frames.items()):
        if img is None:
            problems.append(f"frame {k} shows nothing, but the hitbox kills")
            continue
        over = trap_unfair_by(hit, img, margins)
        if over > over_tol:
            problems.append(f"frame {k}: kills {over:g} px before the pictures touch")
    shown = [img for img in frames.values() if img]
    always = _intersection(shown) if shown and len(shown) == len(frames) else None
    if always:
        for side, past in zip(SIDES, (hit[0] - always[0], hit[1] - always[1],
                                      always[2] - hit[2], always[3] - hit[3])):
            if past > under_tol:
                problems.append(
                    f"hitbox is {past:g} px short of the picture on the {side}"
                    " (drawn in every frame)")
    return problems


def trap_frames(sequence, sequences, graphics_count):
    """Frames (each once, in order) the game shows for a graphics_indexes
    `sequence` name: TRAP_GRAPHICS_INDEXES_0 (an empty span) is a static
    picture, frame 0. Indexes wrap like sprite_index % graphics_count."""
    if sequence == "TRAP_GRAPHICS_INDEXES_0":
        return [0]
    if sequence not in sequences:
        raise ValueError(f"unknown animation sequence {sequence}")
    frames = []
    for index in sequences[sequence]:
        if index % graphics_count not in frames:
            frames.append(index % graphics_count)
    return frames or [0]


def duck_problems(frames, body_w, body_h, offset_y):
    """Why the duck's body does not fit its picture frames, else []. `frames`:
    {frame: opaque box relative to the frame center or None}; the picture is
    drawn offset_y px above the body center. Mirrored too (the duck is
    flipped for the other direction and wall)."""
    problems = []
    half_w, half_h = body_w / 2, body_h / 2
    for k, img in sorted(frames.items()):
        if img is None:
            problems.append(f"frame {k} is empty")
            continue
        top, bottom = img[1] + offset_y, img[3] + offset_y
        feet = bottom - half_h
        if abs(feet) > DUCK_FEET_TOLERANCE:
            problems.append(f"frame {k}: feet are {feet:+g} px off the body's bottom")
        head = -half_h - top
        if head > DUCK_MAX_HEAD_MARGIN:
            problems.append(f"frame {k}: picture reaches {head:g} px above the body")
        if top + half_h > DUCK_BODY_TOLERANCE:
            problems.append(f"frame {k}: body reaches {top + half_h:g} px above the picture")
        for mirrored in (False, True):
            left, right = (-img[2], -img[0]) if mirrored else (img[0], img[2])
            tag = f"frame {k}{' mirrored' if mirrored else ''}"
            for side, past in (("left", left + half_w), ("right", half_w - right)):
                if past > DUCK_BODY_TOLERANCE:
                    problems.append(
                        f"{tag}: body reaches {past:g} px past the picture on the {side}")
                if -past > DUCK_MAX_SIDE_MARGIN:
                    problems.append(
                        f"{tag}: picture reaches {-past:g} px sideways past the body"
                        f" on the {side}")
    return problems


def coverage_problems(sheets, covered, no_body):
    """Sprite sheets that were neither checked nor are listed as bodiless, and
    stale NO_BODY entries."""
    problems = []
    for name in sorted(set(sheets) - set(covered) - set(no_body)):
        problems.append(f"sprite {name}: not checked, and not listed in NO_BODY")
    for name in sorted(no_body):
        if name not in sheets:
            problems.append(f"NO_BODY lists {name}, but there is no such sprite sheet")
        elif name in covered:
            problems.append(f"NO_BODY lists {name}, but it is used as a collision body")
    return problems


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


def sprite_sheet_names(root):
    """Butano names of every sprite (not background) sheet in game/graphics."""
    names = set()
    for path in (Path(root) / "game/graphics").rglob("*.bmp"):
        if json.loads(path.with_suffix(".json").read_text()).get("type") == "sprite":
            names.add(path.stem)
    return names


def _cpp_int(root, relpath, constant):
    text = (Path(root) / relpath).read_text()
    match = re.search(r"\b" + constant + r"\s*=\s*(-?\d+)\s*;", text)
    if match is None:
        raise ValueError(f"{constant} not found in {relpath}")
    return int(match.group(1))


def pogo_box(root):
    """The pogo strike's box relative to the center of its picture (player.cpp
    centers the 16x16 picture on the box): Cfg::Pogo::WIDTH x REACH."""
    width = _cpp_int(root, "game/include/cfg_pogo.h", "WIDTH")
    reach = _cpp_int(root, "game/include/cfg_pogo.h", "REACH")
    return (-width / 2, -reach / 2, width / 2, reach / 2)


def checkpoint_zone(root):
    """(width, height) of the checkpoint's touch zone, centered on the flag."""
    return (_cpp_int(root, "game/src/traps/checkpoint.cpp", "TOUCH_WIDTH"),
            _cpp_int(root, "game/src/traps/checkpoint.cpp", "TOUCH_HEIGHT"))


def _centered(width, height, x=0, y=0):
    return (x - width / 2, y - height / 2, x + width / 2, y + height / 2)


def _frames_of(sprites, name, indexes, scale=1):
    """{frame: opaque box relative to the sprite center, scaled}."""
    frames = {}
    for k in indexes:
        box = sprites.box(name, k)
        frames[k] = scaled_box(box, scale) if box else None
    return frames


def _world_frames(frames, x, y):
    return {k: (_world(box, x, y) if box else None) for k, box in frames.items()}


def _fmt(box):
    return "-" if box is None else "(" + ", ".join(f"{v:g}" for v in box) + ")"


def _report_frames(report, where, hit, frames, margins):
    if report is None:
        return
    numbers = frame_numbers(hit, frames, margins)
    report.append(f"{where}: hit {_fmt(hit)}")
    for k, img in sorted(frames.items()):
        over = numbers[k]
        report.append(f"    frame {k}: picture {_fmt(img)} kills early by "
                      f"{'-' if over is None else f'{over:g}'} px")


def check(root, covered=None, report=None):
    """All problems found in the game under `root`. `covered`: a set that
    receives the name of every sprite that was checked. `report`: a list that
    receives the numbers of every frame of every box."""
    root = Path(root)
    covered = set() if covered is None else covered
    sprites = Sprites(root)
    duck = duck_margins(root)
    level_dir = root / "game/include/level"
    files = sorted(level_dir.glob("levels_world*.h"))
    texts = {path: _ctr.strip_comments(path.read_text()) for path in files}
    sequences = _index_arrays([(level_dir / "levels_common.h").read_text(), *texts.values()])
    problems = []
    scale_at = trap_field_index("sprite_scale")
    boxes = door_boxes(root)

    def sheet_height(name):
        return json.loads(sprites._paths[name].with_suffix(".json").read_text()).get("height", 32)

    # The duck itself.
    covered.add("ente")
    body_w, body_h = cfg_player(root, "BODY_WIDTH"), cfg_player(root, "BODY_HEIGHT")
    duck_frames = _frames_of(sprites, "ente", range(sprites.frame_count("ente")))
    for problem in duck_problems(duck_frames, body_w, body_h, cfg_player(root, "SPRITE_OFFSET_Y")):
        problems.append(f"ente (duck body {body_w}x{body_h}): {problem}")

    # The pogo strike: no duck margin, the strike has no picture of a body.
    covered.add("pogo16x16")
    strike = pogo_box(root)
    pogo_frames = _frames_of(sprites, "pogo16x16", range(sprites.frame_count("pogo16x16")))
    for problem in box_problems(strike, pogo_frames, (0, 0, 0, 0)):
        problems.append(f"pogo16x16 (strike box {_fmt(strike)}): {problem}")
    _report_frames(report, "pogo16x16", strike, pogo_frames, (0, 0, 0, 0))

    # Checkpoints: the touch zone is centered on the flag.
    covered.add("checkpoint16x16")
    zone = _centered(*checkpoint_zone(root))
    flag_frames = _frames_of(sprites, "checkpoint16x16", range(sprites.frame_count("checkpoint16x16")))
    for problem in box_problems(zone, flag_frames, duck):
        problems.append(f"checkpoint16x16 (touch zone {_fmt(zone)}): {problem}")
    _report_frames(report, "checkpoint16x16", zone, flag_frames, duck)

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
                covered.add(name)
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
                if report is not None:
                    report.append(f"{where}: sink {sink:+g} left {left:+g} right {right:+g}")

        for array, body in _ctr.extract_named_arrays(text, "TrapData").items():
            for i, literal in enumerate(_ctr.extract_top_level_literals(body)):
                f = _ctr.top_level_fields(literal)
                x, y, w, h, ox, oy = (_num(v) for v in f[1:7])
                name = _sprite(f[7])
                where = f"{path.name} {array}[{i}] {f[0].strip().split('::')[-1]} {name}"
                if name not in sprites:
                    problems.append(f"{where}: unknown sprite")
                    continue
                covered.add(name)
                sequence = f[9].strip() if len(f) > 9 else "TRAP_GRAPHICS_INDEXES_0"
                try:
                    indexes = trap_frames(sequence, sequences, sprites.frame_count(name))
                except ValueError as error:
                    problems.append(f"{where}: {error}")
                    continue
                scale = _num(f[scale_at]) if len(f) > scale_at else 1
                frames = _world_frames(_frames_of(sprites, name, indexes, scale), x, y)
                hit = (x + ox - w / 2, y + oy - h / 2, x + ox + w / 2, y + oy + h / 2)
                for problem in box_problems(hit, frames, duck):
                    problems.append(f"{where}: {problem}")
                _report_frames(report, where, hit, frames, duck)
                if f[0].strip() == "TrapType::BASE" and name in DOOR_SPRITES:
                    # A fake door must be touched exactly where the real one is.
                    real = boxes.get(sheet_height(name))
                    mine = (ox - w / 2, oy - h / 2, ox + w / 2, oy + h / 2)
                    if real is None or any(abs(a - b) > FAKE_DOOR_TOLERANCE
                                           for a, b in zip(mine, real)):
                        problems.append(
                            f"{where}: fake door box {_fmt(mine)} differs from the real "
                            f"door box {_fmt(real)} (door_logic.h)")

        for array, body in _ctr.extract_named_arrays(text, "TriggerData").items():
            for i, literal in enumerate(_ctr.extract_top_level_literals(body)):
                f = _ctr.top_level_fields(literal)
                name = _sprite(f[7]) if len(f) > 7 else None
                if name is None:
                    continue  # an invisible zone has no picture to match
                where = f"{path.name} {array}[{i}] trigger {name}"
                if name not in sprites:
                    problems.append(f"{where}: unknown sprite")
                    continue
                covered.add(name)
                x, y, w, h = (_num(v) for v in f[:4])
                count = sprites.frame_count(name)
                off = int(_num(f[8])) if len(f) > 8 else 0
                on = int(_num(f[9])) if len(f) > 9 else 1
                frames = _world_frames(
                    _frames_of(sprites, name, sorted({off % count, on % count})), x, y)
                hit = _centered(w, h, x, y)
                for problem in box_problems(hit, frames, duck):
                    problems.append(f"{where}: {problem}")
                _report_frames(report, where, hit, frames, duck)

        for m in re.finditer(r"(?:const|constexpr) LevelData (\w+)\s*=\s*\{", text):
            end = _ctr.find_balanced_block(text, m.end() - 1)
            level = _ctr.top_level_fields(text[m.end():end])
            door = _ctr.top_level_fields(level[9].strip()[1:-1])
            name = _sprite(door[2]) if len(door) > 2 else "door32x32"
            where = f"{path.name} {m.group(1)} door {name}"
            if name not in sprites:
                problems.append(f"{where}: unknown sprite")
                continue
            covered.add(name)
            indexes = [0, 2, 4, 6, 8, 10, 12]  # DOOR32X32_GRAPHICS_INDEXES
            if len(door) > 3:
                indexes = sequences.get(door[3].strip(), [0])
            height = sheet_height(name)
            if height not in boxes:
                problems.append(f"{where}: no DOOR_BOX_{height} in door_logic.h")
                continue
            count = sprites.frame_count(name)
            for k in sorted({i % count for i in indexes}):
                img = sprites.box(name, k)
                reason = door_problem(boxes[height], img) if img else "the frame is empty"
                if reason:
                    problems.append(f"{where} frame {k}: {reason}")

    problems += coverage_problems(sprite_sheet_names(root), covered, NO_BODY)
    return problems


def main(argv, root="."):
    report = [] if "--report" in argv else None
    problems = check(root, report=report)
    for line in report or []:
        print(line)
    for problem in problems:
        print(f"ERROR {problem}", file=sys.stderr)
    print(f"\nHitbox check: {len(problems)} problem(s).")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
