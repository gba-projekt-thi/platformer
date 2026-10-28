#!/usr/bin/env python3
"""
Check_world_index.py

Verifies that WorldIndex::WORLDS (game/include/world_index.h) still describes
the flat levels[] array in game/src/main.cpp - the two are kept in sync by
hand, and a mismatch silently shows the wrong levels in World/Level Select,
unlocks the wrong worlds, and misattributes save data.

Checks:
  - the first world starts at level 0,
  - every world starts right after the previous one ends (no gaps/overlaps),
  - the worlds together cover exactly the levels[] entries,
  - levels[] has no duplicate entries,
  - (warning) levels defined in levels_world*.h but missing from levels[].

Usage (from the repository root):
    python3 tools/Check_world_index.py [main.cpp world_index.h [levels_dir]]

Exit code 0 = consistent, 1 = problems found. Pure text parsing.
"""

import re
import sys
from pathlib import Path


def _strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def parse_levels(main_text):
    text = _strip_comments(main_text)
    m = re.search(r"LevelData\s+levels\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;", text, re.DOTALL)
    if not m:
        raise ValueError("levels[] array not found in main.cpp")
    return re.findall(r"\b(LEVEL_[A-Za-z0-9_]+)\b", m.group(1))


def parse_worlds(world_text):
    text = _strip_comments(world_text)
    m = re.search(r"WORLDS\s*\[\s*\]\s*=\s*\{(.*?)\}\s*;\s*$", text, re.DOTALL | re.MULTILINE)
    body = m.group(1) if m else text
    return [
        (name, int(start), int(count))
        for name, start, count in re.findall(r'\{\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', body)
    ]


def defined_levels(levels_dir):
    names = set()
    for path in Path(levels_dir).glob("levels_world*.h"):
        names.update(
            re.findall(r"(?:const|constexpr)\s+LevelData\s+(LEVEL_[A-Za-z0-9_]+)", path.read_text())
        )
    return names


def check(main_text, world_text, levels_dir=None):
    errors, warnings = [], []
    levels = parse_levels(main_text)
    worlds = parse_worlds(world_text)
    if not worlds:
        return ["no worlds found in world_index.h"], warnings

    dupes = sorted({n for n in levels if levels.count(n) > 1})
    for n in dupes:
        errors.append(f"levels[] lists {n} more than once")

    expected_start = 0
    for name, start, count in worlds:
        if start != expected_start:
            errors.append(
                f'{name}: start_index is {start}, expected {expected_start} '
                f"(worlds must follow each other without gaps or overlaps)"
            )
        if count <= 0:
            errors.append(f"{name}: level_count must be positive (is {count})")
        expected_start = start + count

    covered = sum(c for _, _, c in worlds)
    if covered != len(levels):
        errors.append(
            f"worlds cover {covered} levels but levels[] has {len(levels)} entries"
        )

    if levels_dir is not None:
        for n in sorted(defined_levels(levels_dir) - set(levels)):
            warnings.append(f"{n} is defined but not listed in levels[]")
    return errors, warnings


_SKIP_DIRS = {"extern", "build", ".git", "node_modules"}


def _candidates(root, pattern):
    return [
        p
        for p in sorted(Path(root).glob(pattern))
        if not (_SKIP_DIRS & set(p.relative_to(root).parts))
    ]


def discover(root):
    """Finds (main.cpp, world_index.h, levels_dir) under `root`.

    A repository can contain several main.cpp files (the Butano submodule
    ships examples), so main.cpp is the first candidate - outside extern/ and
    build/ - that actually contains the levels[] array. Returns None entries
    for anything not found.
    """
    known = Path(root) / "game/src/main.cpp", Path(root) / "game/include/world_index.h"
    if known[0].is_file() and known[1].is_file() and (Path(root) / "game/include/level").is_dir():
        return known[0], known[1], Path(root) / "game/include/level"
    main_path = None
    for path in _candidates(root, "**/main.cpp"):
        if re.search(r"LevelData\s+levels\s*\[", path.read_text(errors="ignore")):
            main_path = path
            break
    worlds = _candidates(root, "**/world_index.h")
    levels = _candidates(root, "**/levels_world1.h")
    return (
        main_path,
        worlds[0] if worlds else None,
        levels[0].parent if levels else None,
    )


def main(argv):
    if len(argv) >= 2:
        main_path, world_path = Path(argv[0]), Path(argv[1])
        levels_dir = Path(argv[2]) if len(argv) > 2 else None
    else:
        main_path, world_path, levels_dir = discover(Path("."))
        if main_path is None or world_path is None:
            print(
                "Could not find a main.cpp containing levels[] and a "
                "world_index.h (extern/ and build/ are skipped).",
                file=sys.stderr,
            )
            return 1
    errors, warnings = check(main_path.read_text(), world_path.read_text(), levels_dir)
    for w in warnings:
        print(f"WARN  {w}")
    for e in errors:
        print(f"ERROR {e}", file=sys.stderr)
    print(f"\nWorld index check ({main_path}): {len(errors)} error(s), {len(warnings)} warning(s).")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
