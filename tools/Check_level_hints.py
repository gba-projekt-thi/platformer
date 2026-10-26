#!/usr/bin/env python3
"""
Check_level_hints.py

Validates LevelData::intro_hint strings (the one-time level hint, see
hint_logic.h) in game/include/level/levels_world*.h:

  - at most hint::MAX_CHARS characters (read from hint_logic.h when found,
    else 28) - the variable-width 8x16 font is at most 8px per glyph, so
    28 characters always fit the 240px screen,
  - printable ASCII only (the font has no other glyphs),
  - not empty (use nullptr / omit the field for "no hint").

Usage (from the repository root):
    python3 tools/Check_level_hints.py [file ...]

Exit code 0 = all hints valid, 1 = problems found. Pure text parsing; it
reuses the literal splitter of Check_trigger_references.py.
"""

import importlib.util
import re
import sys
from pathlib import Path

_here = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location(
    "check_trigger_references", _here / "Check_trigger_references.py"
)
_ctr = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_ctr)

DEFAULT_MAX_CHARS = 28
# LevelData field order: platforms, platform_count, triggers, trigger_count,
# traps, trap_count, back_ground, music, player_data, door, world_width,
# world_height, ground_friction, intro_hint
HINT_FIELD = 13


def max_chars(search_root=None):
    path = _ctr.find_game_file(search_root or ".", "hint_logic.h")
    m = path and re.search(r"MAX_CHARS\s*=\s*(\d+)", path.read_text())
    return int(m.group(1)) if m else DEFAULT_MAX_CHARS


def hints_in(path):
    """Yields (level_name, hint_text_or_None) for every LevelData literal."""
    content = _ctr.strip_comments(Path(path).read_text())
    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        ob = m.end()
        if content[ob] != "{":
            continue
        cb = _ctr.find_balanced_block(content, ob)
        fields = _ctr.top_level_fields(content[ob + 1 : cb])
        hint = None
        if len(fields) > HINT_FIELD:
            raw = fields[HINT_FIELD].strip()
            if raw.startswith('"') and raw.endswith('"'):
                hint = raw[1:-1]
            elif raw != "nullptr":
                hint = raw  # not a string literal - flagged by check()
        yield m.group(1), hint


def check(path, limit=DEFAULT_MAX_CHARS):
    errors = []
    for level, hint in hints_in(path):
        if hint is None:
            continue
        where = f"{Path(path).name}: {level}"
        if hint == "":
            errors.append(f"{where} - intro_hint is empty (omit it instead)")
        elif not (hint.startswith("'") or all(32 <= ord(c) <= 126 for c in hint)):
            errors.append(f"{where} - intro_hint has non-ASCII characters")
        if len(hint) > limit:
            errors.append(
                f'{where} - intro_hint "{hint}" is {len(hint)} characters, max {limit}'
            )
    return errors


def main(argv):
    paths = [Path(p) for p in argv] or _ctr.level_files()
    if not paths:
        print("No levels_world*.h files found - nothing to check.", file=sys.stderr)
        return 1
    limit = max_chars()
    all_errors = []
    for path in paths:
        all_errors.extend(check(path, limit))
    for e in all_errors:
        print(f"ERROR {e}", file=sys.stderr)
    print(f"\nChecked {len(paths)} file(s): {len(all_errors)} error(s).")
    return 1 if all_errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
