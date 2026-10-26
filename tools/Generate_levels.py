#!/usr/bin/env python3
"""
Generate_levels.py

Generates game/include/level/levels_worldN.h from game/levels/worldN.json.
The JSON files are the level data; the headers are build input and must not
be edited by hand.

JSON layout (one file per world):

    {
      "note": "comment at the top of the header (optional)",
      "end_note": "comment at the end of the header (optional)",
      "levels": [
        {
          "name": "LEVEL_0",                      // the LevelData constant
          "note": "comment above the level's arrays (optional)",
          "data_note": "comment right above the LevelData (optional)",
          "platforms":   {"name": "level0_platforms", "note": "...", "items": [...]},
          "triggers":    {...} or omitted,
          "traps":       {...} or omitted,
          "checkpoints": {...} or omitted,
          "background": "world1", "music": "world1",
          "spawn": [-96, 42],
          "door": {"x": 32, "y": 4, ...DoorData fields},
                                                  // flee_path: [[x, y], ...]
          "world_width": 624, "world_height": 160, "ground_friction": 1,
          "intro_hint": "New: Dash with B",       // optional, like the rest
          "field_notes": {"door": "comment above the door field", ...}
        }
      ]
    }

Items carry the fields of PlatformData / TriggerData / TrapData /
CheckpointData by name (see level_structure.h) plus an optional "note"
(comment above it) and "blank_before" (empty line above it). Field order and
defaults are read from level_structure.h. Values: numbers, true/false, null
(nullptr), strings for the names (trigger names, hints); sprites by item name
("bubbles"), trap types and trigger modes / actions by their enumerator
("MOVING", "Toggle"), spans and paths by their constant (levels_common.h);
any other string is written as a C++ expression ("Cfg::ZOrder::DOOR").

Usage (from the repository root):
    python3 tools/Generate_levels.py           write every header
    python3 tools/Generate_levels.py --check   exit 1 if a header is out of date

--check compares without whitespace and comments, so the header may be
formatted by any clang-format version.
"""

import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEVELS_DIR = ROOT / "game/levels"
HEADER_DIR = ROOT / "game/include/level"
STRUCTURE = HEADER_DIR / "level_structure.h"

STRING_FIELDS = {
    ("PlatformData", "trigger_name"),
    ("TriggerData", "name"),
    ("TrapData", "trigger_name"),
    ("LevelData", "intro_hint"),
}
PREFIXES = {
    ("PlatformData", "sprite"): "bn::sprite_items::",
    ("TrapData", "sprite"): "bn::sprite_items::",
    ("DoorData", "sprite"): "bn::sprite_items::",
    ("TriggerData", "sprite"): "&bn::sprite_items::",
    ("TrapData", "type"): "TrapType::",
    ("TriggerData", "mode"): "TriggerMode::",
    ("TriggerData", "action"): "TriggerAction::",
}
META = {"note", "blank_before"}
# Span fields without a default initializer: their empty value.
SPAN_DEFAULTS = {
    ("TrapData", "graphic_indexes"): "TRAP_GRAPHICS_INDEXES_0",
    ("TrapData", "path"): "NO_PATH_TRAP",
}

# LevelData fields in order, with the JSON key that fills them.
LEVEL_KEYS = [
    ("platforms", "platforms"), ("platform_count", "platforms"),
    ("triggers", "triggers"), ("trigger_count", "triggers"),
    ("traps", "traps"), ("trap_count", "traps"),
    ("back_ground", "background"), ("music", "music"),
    ("player_data", "spawn"), ("door", "door"),
    ("world_width", "world_width"), ("world_height", "world_height"),
    ("ground_friction", "ground_friction"), ("intro_hint", "intro_hint"),
    ("checkpoints", "checkpoints"), ("checkpoint_count", "checkpoints"),
]
ARRAYS = [("platforms", "PlatformData"), ("triggers", "TriggerData"),
          ("traps", "TrapData"), ("checkpoints", "CheckpointData")]


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def read_structs(path=STRUCTURE):
    """{struct: [(field, default C++ text or "")]} in declaration order."""
    text = strip_comments(Path(path).read_text())
    structs = {}
    for name in ("PlatformData", "TriggerData", "TrapData", "PlayerData",
                 "DoorData", "CheckpointData", "LevelData"):
        body = re.search(r"struct " + name + r" \{(.*?)\n\};", text, re.S).group(1)
        fields = re.findall(r"^\s*[\w:<> ,*&]+?\s+(\w+)(?:\s*=\s*([^;]+))?;", body, re.M)
        structs[name] = [(field, default.strip()) for field, default in fields]
    return structs


def load(path):
    return json.loads(Path(path).read_text())


def dumps(data, indent=0):
    """JSON in the layout of game/levels/: one line per platform, trigger,
    trap or checkpoint, short lists inline."""
    pad, inner = "  " * indent, "  " * (indent + 1)
    if isinstance(data, dict):
        lines = []
        for key, value in data.items():
            if key == "items":
                body = ",\n".join(inner + "  " + json.dumps(item, ensure_ascii=False)
                                   for item in value)
                lines.append(f'{inner}"items": [\n{body}\n{inner}]')
            else:
                lines.append(f"{inner}{json.dumps(key)}: {dumps(value, indent + 1)}")
        return "{\n" + ",\n".join(lines) + "\n" + pad + "}"
    if isinstance(data, list) and any(isinstance(v, (dict, list)) for v in data):
        return "[\n" + ",\n".join(inner + dumps(v, indent + 1) for v in data) + "\n" + pad + "]"
    return json.dumps(data, ensure_ascii=False)


def comment(note, indent=""):
    if not note:
        return ""
    return "".join(f"{indent}// {line}".rstrip() + "\n" for line in note.split("\n"))


def value(struct, field, v):
    if v is None:
        return "nullptr"
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (int, float)):
        return repr(v)
    if (struct, field) in STRING_FIELDS:
        return json.dumps(v)
    return PREFIXES.get((struct, field), "") + v


def positional(struct, item, structs):
    """The fields of `item` in declaration order, up to the last one given;
    skipped ones in between get their default."""
    fields = structs[struct]
    # Up to the last field given, and always past every field without a
    # default member initializer (leaving one out of an aggregate is a
    # -Wmissing-field-initializers warning).
    needed = [i for i, (f, d) in enumerate(fields) if f in item or not d]
    out = []
    for field, default in fields[: (needed[-1] + 1) if needed else 0]:
        if field in item:
            out.append(value(struct, field, item[field]))
        elif default or (struct, field) in SPAN_DEFAULTS:
            out.append(default or SPAN_DEFAULTS[(struct, field)])
        else:
            raise ValueError(f"{struct}: required field '{field}' missing in {item}")
    return "{" + ", ".join(out) + "}"


def _array(struct, array, structs):
    definitions = ""
    items = []
    for i, item in enumerate(array["items"]):
        definition, item = _flee_path(item, f"{array['name']}_{i}_flee_path")
        definitions += definition
        text = ""
        if item.get("blank_before"):
            text += "\n"
        text += comment(item.get("note"), "    ")
        text += "    " + positional(struct, {k: v for k, v in item.items() if k not in META}, structs)
        items.append(text)
    out = definitions + comment(array.get("note"))
    out += f"constexpr {struct} {array['name']}[] = {{\n"
    out += ",\n".join(items)
    if array.get("trailing_note"):
        out += "\n" + comment(array["trailing_note"], "    ").rstrip("\n")
    return out + "};\n"


def _flee_path(item, name):
    """(definition, item) - a running door's or platform's flee_path
    ([[x, y], ...]) becomes its own bn::fixed_point array called `name`."""
    path = item.get("flee_path")
    if not isinstance(path, list):
        return "", item
    item = dict(item, flee_path=name)
    points = ", ".join(f"{{{value('DoorData', 'x', x)}, {value('DoorData', 'y', y)}}}" for x, y in path)
    return f"constexpr bn::fixed_point {name}[] = {{{points}}};\n\n", item


def _door(level):
    """(definitions to put before the level, DoorData fields)."""
    return _flee_path(level["door"], f"{level['name']}_door_flee_path")


def _level(level, structs):
    definitions, door = _door(level)
    parts = {}
    for name, _ in ARRAYS:
        array = level.get(name)
        parts[name] = (array["name"], f"sizeof({array['name']}) / sizeof({array['name']}[0])") \
            if array else ("nullptr", "0")
    values = {
        "platforms": parts["platforms"][0], "platform_count": parts["platforms"][1],
        "triggers": parts["triggers"][0], "trigger_count": parts["triggers"][1],
        "traps": parts["traps"][0], "trap_count": parts["traps"][1],
        "checkpoints": parts["checkpoints"][0], "checkpoint_count": parts["checkpoints"][1],
        "back_ground": "bn::regular_bg_items::" + level["background"],
        "music": "bn::music_items::" + level["music"],
        "player_data": "{" + ", ".join(value("PlayerData", f, v) for f, v in
                                       zip(("x", "y"), level["spawn"])) + "}",
        "door": positional("DoorData", door, structs),
    }
    for key in ("world_width", "world_height", "ground_friction", "intro_hint"):
        if key in level:
            values[key] = value("LevelData", key, level[key])

    defaults = dict(structs["LevelData"])
    present = [i for i, (_, key) in enumerate(LEVEL_KEYS) if key in level]
    notes = level.get("field_notes", {})
    lines = []
    seen_keys = set()
    for field, key in LEVEL_KEYS[: present[-1] + 1]:
        text = ""
        if key not in seen_keys:
            text += comment(notes.get(key), "    ")
            seen_keys.add(key)
        text += "    " + (values[field] if field in values else defaults[field])
        lines.append(text)
    out = definitions + comment(level.get("data_note"))
    out += f"const LevelData {level['name']} = {{\n" + ",\n".join(lines) + "};\n"
    return out


def includes(data):
    sprites, backgrounds = set(), set()
    for level in data["levels"]:
        backgrounds.add(level["background"])
        for name, _ in ARRAYS:
            for item in (level.get(name) or {}).get("items", []):
                if isinstance(item.get("sprite"), str):
                    sprites.add(item["sprite"])
        if isinstance(level["door"].get("sprite"), str):
            sprites.add(level["door"]["sprite"])
    names = ["bn_music_items.h"]
    names += [f"bn_regular_bg_items_{b}.h" for b in sorted(backgrounds)]
    names += [f"bn_sprite_items_{s}.h" for s in sorted(sprites)]
    out = "".join(f'#include "{n}"\n' for n in names)
    return out + '\n#include "level_structure.h"\n#include "levels_common.h"\n'


def generate(data, structs, source="game/levels/<world>.json"):
    out = "#pragma once\n\n"
    out += f"// Generated by tools/Generate_levels.py from {source} - edit the JSON,\n"
    out += "// not this file.\n\n"
    out += includes(data) + "\n"
    if data.get("note"):
        out += comment(data["note"]) + "\n"
    for level in data["levels"]:
        out += comment(level.get("note"))
        for name, struct in ARRAYS:
            if level.get(name):
                out += _array(struct, level[name], structs) + "\n"
        out += _level(level, structs) + "\n"
    if data.get("end_note"):
        out += comment(data["end_note"])
    return out.rstrip("\n") + "\n"


def _squash(text):
    return re.sub(r"\s+", "", strip_comments(text))


def matches(header, data, structs):
    """The header holds the same code as the JSON (whitespace and comments
    aside)."""
    return Path(header).exists() and \
        _squash(Path(header).read_text()) == _squash(generate(data, structs))


def main(argv):
    structs = read_structs()
    sources = sorted(LEVELS_DIR.glob("world*.json"))
    if not sources:
        print(f"No world*.json in {LEVELS_DIR}.", file=sys.stderr)
        return 1
    stale = []
    for source in sources:
        header = HEADER_DIR / f"levels_{source.stem}.h"
        data = load(source)
        if "--check" in argv:
            if not matches(header, data, structs):
                stale.append(header)
            continue
        header.write_text(generate(data, structs, f"game/levels/{source.name}"))
        if shutil.which("clang-format"):
            subprocess.run(["clang-format", "-i", str(header)], check=True)
        print(f"wrote {header.relative_to(ROOT)}")
    for header in stale:
        print(f"ERROR {header.relative_to(ROOT)} does not match its JSON: "
              "run python3 tools/Generate_levels.py", file=sys.stderr)
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
