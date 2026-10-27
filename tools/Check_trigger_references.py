#!/usr/bin/env python3
"""
Check_trigger_references.py

Statically validates every TrapData::trigger_name a MOVING/PATH/CYCLE/BOSS trap - and
every PlatformData::trigger_name a switched platform - references against the TriggerData::name entries actually defined for
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
    python3 tools/Check_trigger_references.py [file ...]

With no arguments, scans every game/include/level/levels_world*.h found
under the repository root (wherever this script is invoked from). Exit
code 0 means every trigger_name reference resolved; non-zero means at
least one dangling reference was found (printed with file/level/name
detail). Also warns (non-fatal) about triggers that are defined but never
referenced by any trap or platform in their level - not a runtime bug, but often a
sign of a rename that missed a spot.

Pure text/data parsing - no C++ compiler involved, so this runs anywhere
Python 3 is available (already a build-time dependency of this project;
see docker/Dockerfile and Tilemap.md).
"""

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


# Positions in a TriggerData literal (level_structure.h): x, y, width, height,
# default_on, name, mode, sprite, sprite_index_off, sprite_index_on,
# delay_frames, action, follows.
TRIGGER_NAME_FIELD = 5
TRIGGER_FOLLOWS_FIELD = 12


def quoted_field(fields, index):
    """The string at position `index` of a literal, or None (missing,
    nullptr)."""
    if index < len(fields):
        f = fields[index]
        if f.startswith('"') and f.endswith('"'):
            return f[1:-1]
    return None


def check_follows(follows, defined_names, where, trigger_array_name):
    """Errors for TriggerData::follows (delayed trap chains): the source must
    be a trigger of the same level, not the trigger itself, and following
    must never lead in a circle (such a chain could never fire)."""
    errors = []
    for name, source in sorted(follows.items(), key=lambda kv: str(kv[0])):
        if source == name:
            errors.append(f'{where} - trigger "{name}" follows itself')
        elif source not in defined_names:
            errors.append(
                f'{where} - trigger "{name}" follows "{source}", '
                f"not found in '{trigger_array_name}'"
            )
    reported = set()
    for start in sorted(n for n in follows if n is not None):
        path = []
        current = start
        while current in follows and current not in path:
            path.append(current)
            current = follows[current]
        if current not in path:
            continue  # the chain ends at an ordinary trigger
        cycle = path[path.index(current):]
        if len(cycle) == 1 or frozenset(cycle) in reported:
            continue  # following itself is reported above
        reported.add(frozenset(cycle))
        first = cycle.index(min(cycle))
        cycle = cycle[first:] + cycle[:first]
        chain = " -> ".join(f'"{n}"' for n in cycle + [cycle[0]])
        errors.append(f"{where} - follows forms a cycle: {chain}")
    return errors


def check_file(path: Path):
    """Returns (errors, warnings) - lists of human-readable strings."""
    content = strip_comments(path.read_text())

    trigger_arrays = extract_named_arrays(content, "TriggerData")
    trap_arrays = extract_named_arrays(content, "TrapData")
    platform_arrays = extract_named_arrays(content, "PlatformData")

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
        follows = {}  # follower name -> source name (TriggerData::follows)
        if trigger_array_name != "nullptr":
            body = trigger_arrays.get(trigger_array_name)
            if body is None:
                errors.append(
                    f"{path.name}: {level_name} references trigger array "
                    f"'{trigger_array_name}', which is not defined in this file"
                )
            else:
                for lit in extract_top_level_literals(body):
                    fields = top_level_fields(lit)
                    name = quoted_field(fields, TRIGGER_NAME_FIELD)
                    if name is not None:
                        defined_names.add(name)
                    source = quoted_field(fields, TRIGGER_FOLLOWS_FIELD)
                    if source is not None:
                        follows[name] = source

        referenced_names = set(follows.values())
        errors.extend(
            check_follows(follows, defined_names, f"{path.name}: {level_name}",
                          trigger_array_name)
        )
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
                    if trap_type in ("MOVING", "PATH", "CYCLE", "BOSS"):
                        names = quoted_strings_in(fields)
                        referenced_names.update(names)
                        for n in names:
                            if n not in defined_names:
                                errors.append(
                                    f"{path.name}: {level_name} - trap in "
                                    f"'{trap_array_name}' references trigger_name "
                                    f'"{n}", not found in \'{trigger_array_name}\''
                                )

        # PlatformData::trigger_name binds a platform to a trigger (a switch
        # opening/closing it). Platform literals have no other quoted field,
        # so any quoted string in one is a trigger name.
        platform_array_name = level_fields[0]
        if platform_array_name != "nullptr":
            body = platform_arrays.get(platform_array_name)
            if body is not None:
                for lit in extract_top_level_literals(body):
                    for n in quoted_strings_in(top_level_fields(lit)):
                        referenced_names.add(n)
                        if n not in defined_names:
                            errors.append(
                                f"{path.name}: {level_name} - platform in "
                                f"'{platform_array_name}' references trigger_name "
                                f'"{n}", not found in \'{trigger_array_name}\''
                            )

        unused = defined_names - referenced_names
        for n in sorted(unused):
            warnings.append(
                f"{path.name}: {level_name} - trigger \"{n}\" in "
                f"'{trigger_array_name}' is never referenced by any "
                f"MOVING/PATH/CYCLE/BOSS trap or platform in this level"
            )

    return errors, warnings


_GAME_DIRS = ("game/include", "game/include/level", "game/src", ".")


def find_game_file(root, name):
    """The game file `name` under `root`: looked up in the game's own folders
    first, searched recursively only if it is not there (a recursive search
    walks extern/ and build/ too and takes seconds). None if missing."""
    root = Path(root)
    for folder in _GAME_DIRS:
        if (root / folder / name).is_file():
            return root / folder / name
    matches = sorted(root.glob(f"**/{name}"))
    return matches[0] if matches else None


def level_files(root="."):
    """Every levels_world*.h, from game/include/level/ when it exists."""
    level_dir = Path(root) / "game/include/level"
    if level_dir.is_dir():
        return sorted(level_dir.glob("levels_world*.h"))
    return sorted(Path(root).glob("**/levels_world*.h"))


def main(argv):
    if argv:
        paths = [Path(p) for p in argv]
    else:
        paths = level_files()

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