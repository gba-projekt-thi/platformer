#!/usr/bin/env python3
"""
check_trigger_references.py

Statically validates every TrapData::trigger_name a MOVING/PATH trap
references against the TriggerData::name entries actually defined for
THAT SAME LEVEL - the exact per-level lookup
LevelManager::get_trigger_by_name() performs at runtime, but checked here
without compiling or running the game.

Why per-level, not just per-file: a levels_worldN.h file typically defines
several levels (e.g. LEVEL_0, LEVEL_1, WORLD1_SCROLL, WORLD1_BOSS all live
in levels_world1.h), each with its own separate trigger/trap array pair.
A trap in one level accidentally referencing a trigger name that only
exists in a *different* level in the same file would still be a real bug
(get_trigger_by_name() only searches that level's own _triggers), so this
script follows each LevelData literal's actual triggers/traps array
references rather than just checking "somewhere in this file".

This is the kind of bug that motivated writing this script in the first
place: a trap referencing a trigger that doesn't exist for its level used
to be silently swallowed by the old raw trigger_index lookup (falling back
to trigger 0 on an out-of-bounds index) - named lookup logs an error at
runtime instead, but only if that trap is actually loaded during play.
This check catches it at commit/CI time instead, without needing to reach
that specific level in the emulator.

Usage:
    python3 tools/check_trigger_references.py [file ...]

With no arguments, scans every game/include/level/levels_world*.h found
under the repository root (wherever this script is invoked from). Exit
code 0 means every trigger_name reference resolved; non-zero means at
least one dangling reference was found (printed with file/level/name
detail). Also warns (non-fatal) about triggers that are defined but never
referenced by any trap in their level - not a runtime bug, but often a
sign of a rename that missed a spot.

Pure text/data parsing - no C++ compiler involved, so this runs anywhere
Python 3 is available (already a build-time dependency of this project;
see docker/Dockerfile and Tilemap.md).
"""

import glob
import re
import sys
from pathlib import Path


def strip_comments(text: str) -> str:
    return re.sub(r"//[^\n]*", "", text)


def find_balanced_block(text: str, start: int) -> int:
    """Given the index of an opening '{' in text, return the index of its
    matching closing '}'."""
    assert text[start] == "{"
    depth = 0
    i = start
    while i < len(text):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced braces")


def extract_named_arrays(content: str, keyword: str) -> dict:
    """Finds every `constexpr <keyword> <name>[] = { ... };` and returns
    {name: array_body_including_outer_braces}."""
    arrays = {}
    for m in re.finditer(rf"constexpr {keyword} ([A-Za-z0-9_]+)\[\]\s*=\s*", content):
        name = m.group(1)
        open_brace = m.end()
        if content[open_brace] != "{":
            continue
        close_brace = find_balanced_block(content, open_brace)
        arrays[name] = content[open_brace : close_brace + 1]
    return arrays


def extract_top_level_literals(array_body: str):
    """array_body includes its own outer braces: '{ {..}, {..}, ... }'.
    Returns the text of each direct child {...} literal, without its own
    braces."""
    assert array_body[0] == "{"
    literals = []
    depth = 0
    start = None
    for i, c in enumerate(array_body):
        if c == "{":
            depth += 1
            if depth == 2:
                start = i + 1
        elif c == "}":
            if depth == 2:
                literals.append(array_body[start:i])
            depth -= 1
    return literals


def top_level_fields(literal: str):
    """Splits a struct literal's body on top-level commas only (commas
    inside nested {..} or (..) - e.g. sizeof(x)/sizeof(x[0]) - don't
    split)."""
    depth = 0
    fields = []
    current = ""
    for c in literal:
        if c in "{(":
            depth += 1
        elif c in "})":
            depth -= 1
        if c == "," and depth == 0:
            fields.append(current.strip())
            current = ""
        else:
            current += c
    if current.strip():
        fields.append(current.strip())
    return fields


def quoted_strings_in(fields):
    return [f[1:-1] for f in fields if f.startswith('"') and f.endswith('"')]


def check_file(path: Path):
    """Returns (errors, warnings) - lists of human-readable strings."""
    content = strip_comments(path.read_text())

    trigger_arrays = extract_named_arrays(content, "TriggerData")
    trap_arrays = extract_named_arrays(content, "TrapData")

    errors = []
    warnings = []

    for m in re.finditer(r"(?:const|constexpr) LevelData ([A-Za-z0-9_]+)\s*=\s*", content):
        level_name = m.group(1)
        open_brace = m.end()
        if content[open_brace] != "{":
            continue
        close_brace = find_balanced_block(content, open_brace)
        level_fields = top_level_fields(content[open_brace + 1 : close_brace])

        # LevelData field order: platforms(0), platform_count(1),
        # triggers(2), trigger_count(3), traps(4), trap_count(5), ...
        if len(level_fields) < 5:
            continue
        trigger_array_name = level_fields[2]
        trap_array_name = level_fields[4]

        defined_names = set()
        if trigger_array_name != "nullptr":
            body = trigger_arrays.get(trigger_array_name)
            if body is None:
                errors.append(
                    f"{path.name}: {level_name} references trigger array "
                    f"'{trigger_array_name}', which is not defined in this file"
                )
            else:
                for lit in extract_top_level_literals(body):
                    defined_names.update(quoted_strings_in(top_level_fields(lit)))

        referenced_names = set()
        if trap_array_name != "nullptr":
            body = trap_arrays.get(trap_array_name)
            if body is None:
                errors.append(
                    f"{path.name}: {level_name} references trap array "
                    f"'{trap_array_name}', which is not defined in this file"
                )
            else:
                for lit in extract_top_level_literals(body):
                    fields = top_level_fields(lit)
                    if not fields or "::" not in fields[0]:
                        continue
                    trap_type = fields[0].split("::")[-1]
                    if trap_type in ("MOVING", "PATH"):
                        names = quoted_strings_in(fields)
                        referenced_names.update(names)
                        for n in names:
                            if n not in defined_names:
                                errors.append(
                                    f"{path.name}: {level_name} - trap in "
                                    f"'{trap_array_name}' references trigger_name "
                                    f'"{n}", not found in \'{trigger_array_name}\''
                                )

        unused = defined_names - referenced_names
        for n in sorted(unused):
            warnings.append(
                f"{path.name}: {level_name} - trigger \"{n}\" in "
                f"'{trigger_array_name}' is never referenced by any "
                f"MOVING/PATH trap in this level"
            )

    return errors, warnings


def main(argv):
    if argv:
        paths = [Path(p) for p in argv]
    else:
        paths = sorted(Path(p) for p in glob.glob("**/levels_world*.h", recursive=True))

    if not paths:
        print("No levels_world*.h files found - nothing to check.", file=sys.stderr)
        return 1

    all_errors = []
    all_warnings = []
    for path in paths:
        errors, warnings = check_file(path)
        all_errors.extend(errors)
        all_warnings.extend(warnings)

    for w in all_warnings:
        print(f"WARN  {w}")
    for e in all_errors:
        print(f"ERROR {e}", file=sys.stderr)

    print(
        f"\nChecked {len(paths)} file(s): {len(all_errors)} error(s), "
        f"{len(all_warnings)} warning(s)."
    )
    return 1 if all_errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))