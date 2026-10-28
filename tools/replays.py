#!/usr/bin/env python3
"""
replays.py

Emulator tests that play every level to its door with recorded buttons.

Recording (once per level, by playing it):
    make record                         builds platformer.record/platformer.gba
    (play the level(s) to the door in mGBA, then quit)
    python3 tools/replays.py import platformer.record/platformer.sav
        -> game/replays/<LEVEL>.json, one file per level that was finished

Playing back:
    make replay                         generate + build + run, see below
    python3 tools/replays.py generate [--check]
        game/replays/*.json -> game/include/replay_data.h
    python3 tools/replays.py run platformer.replay/platformer.gba [--seconds N]
        runs the replay ROM headless (mGBA without frame limit) and exits 1
        unless every recorded level reached its door

The recording build stores each level's last finished attempt in SRAM behind
the save slots (layout in game/include/replay.h); a Restart Level from the
pause menu starts the recording of that level over. Buttons are recorded per
movement step, so pauses and the death freeze do not matter.

A level may change with the deaths in it (death windows, a door that moves),
and a Restart Level is a death: a recording keeps the deaths in the level it
began with ("deaths" in the JSON, 0 when left out), and the replay ROM sets
the level up as it stands after that many deaths before it plays.

A level can have one recording per death state: game/replays/<LEVEL>.json
(deaths 0) and game/replays/<LEVEL>@<deaths>.json. The ROM and its log name
them LEVEL and LEVEL@3.

    python3 tools/replays.py layouts [--strict]
        lists every layout a level can have by death count (from the death
        windows and the moving door in game/levels/*.json) and which of them
        have no recording; --strict exits 1 if any is missing.
"""

import importlib.util
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPLAY_DIR = ROOT / "game/replays"
HEADER = ROOT / "game/include/replay_data.h"

# Must match game/include/replay.h (checked by tools/test_replays.py).
SRAM_OFFSET = 0x2000
BLOCK_SIZE = 0x300
HEADER_SIZE = 6
MAGIC = 0x54
FLAG_TRUNCATED = 0x01


def _campaign_levels():
    spec = importlib.util.spec_from_file_location(
        "check_world_index", ROOT / "tools/Check_world_index.py")
    cwi = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cwi)
    return cwi.parse_levels((ROOT / "game/src/main.cpp").read_text())


# ---- import -----------------------------------------------------------------
def read_sav(data, level_count):
    """{level index: ([(buttons, count), ...], truncated, deaths in the level
    at the start)} from a .sav image."""
    recordings = {}
    for index in range(level_count):
        offset = SRAM_OFFSET + index * BLOCK_SIZE
        if offset + HEADER_SIZE > len(data):
            break
        magic, flags, count, deaths = struct.unpack_from("<BBHH", data, offset)
        if magic != MAGIC or count * 2 + HEADER_SIZE > BLOCK_SIZE:
            continue
        runs = []
        for k in range(count):
            packed, = struct.unpack_from("<H", data, offset + HEADER_SIZE + 2 * k)
            runs.append((packed >> 10, packed & 0x3FF))
        recordings[index] = (runs, bool(flags & FLAG_TRUNCATED), deaths)
    return recordings


def replay_path(name, deaths):
    """game/replays/<LEVEL>.json, <LEVEL>@<deaths>.json after deaths."""
    return REPLAY_DIR / (f"{name}@{deaths}.json" if deaths else f"{name}.json")


def replay_json(name, runs, deaths):
    """The JSON of one recording; "deaths" only when it began after some."""
    data = {"level": name, "runs": runs}
    if deaths:
        data["deaths"] = deaths
    return data


def import_sav(path):
    levels = _campaign_levels()
    recordings = read_sav(Path(path).read_bytes(), len(levels))
    if not recordings:
        print(f"No recording in {path}.", file=sys.stderr)
        return 1
    REPLAY_DIR.mkdir(exist_ok=True)
    for index, (runs, truncated, deaths) in sorted(recordings.items()):
        name = levels[index]
        if truncated:
            print(f"SKIP  {name}: the recording did not fit (too many button changes)")
            continue
        target = replay_path(name, deaths)
        target.write_text(json.dumps(replay_json(name, runs, deaths)) + "\n")
        print(f"wrote {target.relative_to(ROOT)} ({sum(c for _, c in runs)} steps)")
    return 0


# ---- generate -----------------------------------------------------------------
def load_replays():
    """({(level, deaths): runs}, {(level, deaths): [x, y]} for the replays
    that start elsewhere). `deaths`: the deaths in the level the recording
    begins with (the level as it stands then); one recording per pair."""
    replays, starts = {}, {}
    for path in sorted(REPLAY_DIR.glob("*.json")):
        data = json.loads(path.read_text())
        key = (data["level"], data.get("deaths", 0))
        if "@" in path.stem and path.stem != f"{key[0]}@{key[1]}":
            raise ValueError(f"{path.name}: the file name says another level / deaths than the JSON")
        if key in replays:
            raise ValueError(f"{path.name}: {key[0]} has a second recording for {key[1]} deaths")
        replays[key] = data["runs"]
        if "start" in data:
            starts[key] = data["start"]
    return replays, starts


def entry_name(level, deaths):
    return f"{level}@{deaths}" if deaths else level


def generate(levels, replays, starts=None):
    """`replays`: {(level, deaths): runs}. `starts`: optional {(level, deaths):
    [x, y]} - the duck starts there instead of at the level's spawn (a boss
    fight tested on its own). A recording with deaths > 0 is played with the
    level set up as it stands after that many deaths in it."""
    starts = starts or {}
    out = ["#pragma once", "",
           "// Generated by tools/replays.py from game/replays/*.json - record with",
           "// `make record` and `tools/replays.py import`, do not edit by hand.", "",
           '#include "replay.h"', "", "namespace replay_data {", "",
           "struct Entry {", "    const char* name;  // LEVEL, or LEVEL@deaths",
           "    int level;  // index in main.cpp's levels[]",
           "    const replay::Run* runs;",
           "    int run_count;", "    int steps;",
           "    bool has_start;  // the duck starts at (start_x, start_y)",
           "    int start_x;", "    int start_y;",
           "    unsigned deaths;  // deaths in the level before the first step",
           "};", ""]
    entries = []
    for index, name in enumerate(levels):
        states = sorted(d for (level, d) in replays if level == name)
        for deaths in states:
            runs = replays[(name, deaths)]
            array = f"{name}_AT{deaths}_RUNS" if deaths else f"{name}_RUNS"
            body = ", ".join(f"{{{b}, {c}}}" for b, c in runs)
            out.append(f"constexpr replay::Run {array}[] = {{{body}}};")
            start = starts.get((name, deaths))
            where = f"true, {start[0]}, {start[1]}" if start else "false, 0, 0"
            entries.append(
                f'    {{"{entry_name(name, deaths)}", {index}, {array}, {len(runs)}, '
                f'{sum(c for _, c in runs)}, {where}, {deaths}}}')
        if not states:
            entries.append(f'    {{"{name}", {index}, nullptr, 0, 0, false, 0, 0, 0}}')
    out += ["", "// One entry per recording (a level without one: one entry without runs),",
            "// in play order (main.cpp's levels[]), a level's death states by deaths.",
            "inline constexpr Entry ENTRIES[] = {"]
    out.append(",\n".join(entries))
    out += ["};", "", "inline constexpr int ENTRY_COUNT = sizeof(ENTRIES) / sizeof(ENTRIES[0]);",
            "", "}  // namespace replay_data", ""]
    return "\n".join(out)


# ---- layouts ------------------------------------------------------------------
def layout_deaths(level):
    """The deaths at which a level's layout changes, 0 first: every
    from_death / until_death of a platform or trap, and where the door moves
    (positions[min(deaths // every, count - 1)], attempt_variant.h)."""
    points = {0}
    for key in ("platforms", "traps"):
        for item in (level.get(key) or {}).get("items", []):
            points.update(d for d in (item.get("from_death", 0), item.get("until_death", 0)) if d)
    door = level.get("door") or {}
    every, count = door.get("attempt_every", 0), len(door.get("attempt_positions") or [])
    if every and count > 1:
        points.update(every * k for k in range(1, count))
    return sorted(points)


def level_layouts():
    """{level: [deaths at which its layout starts]} for every level."""
    layouts = {}
    for path in sorted((ROOT / "game/levels").glob("world*.json")):
        for level in json.loads(path.read_text())["levels"]:
            layouts[level["name"]] = layout_deaths(level)
    return layouts


def missing_layouts(layouts, recorded):
    """[(level, deaths)] of the layouts no recording covers. A recording with
    d deaths plays the layout that starts at the largest point <= d."""
    covered = set()
    for level, deaths in recorded:
        starts = [p for p in layouts.get(level, []) if p <= deaths]
        if starts:
            covered.add((level, max(starts)))
    return [(level, p) for level, points in layouts.items() for p in points
            if (level, p) not in covered]


def check_layouts(strict):
    layouts = level_layouts()
    missing = missing_layouts(layouts, load_replays()[0])
    for level, points in layouts.items():
        if len(points) > 1:
            gone = {p for lv, p in missing if lv == level}
            print(f"{level}: " + ", ".join(
                f"@{p}" + (" (NO RECORDING)" if p in gone else "") for p in points))
    for level, p in missing:
        if len(layouts[level]) == 1:
            print(f"{level}: no recording")
    if missing:
        print(f"{'ERROR' if strict else 'WARNING'} {len(missing)} layout(s) without a recording",
              file=sys.stderr)
    return 1 if missing and strict else 0


def _squash(text):
    return re.sub(r"\s+", "", text)


def write_header(check):
    text = generate(_campaign_levels(), *load_replays())
    # Whitespace does not count: the committed header is clang-formatted.
    up_to_date = HEADER.exists() and _squash(HEADER.read_text()) == _squash(text)
    if check:
        if not up_to_date:
            print("ERROR game/include/replay_data.h does not match game/replays/: "
                  "run python3 tools/replays.py generate", file=sys.stderr)
            return 1
        return 0
    if not up_to_date:
        HEADER.write_text(text)
        # Formatted as committed (the pre-commit hook would do it otherwise).
        if shutil.which("clang-format"):
            subprocess.run(["clang-format", "-i", str(HEADER)], check=True)
        print(f"wrote {HEADER.name}")
    return 0


# ---- run ---------------------------------------------------------------------
LINE = re.compile(r"REPLAY (\d+) ([\w@]+) (pass|FAIL|none)")


def parse_log(text):
    """([(level, result)], whether the run reached its end)."""
    results = [(m.group(2), m.group(3)) for m in LINE.finditer(text)]
    return results, "REPLAY DONE" in text


def run(rom, seconds):
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    runtime = Path(tempfile.mkdtemp(prefix="mgba-"))
    env.setdefault("XDG_RUNTIME_DIR", str(runtime))
    # stdbuf: mGBA would buffer its log until it exits.
    command = ["stdbuf", "-oL", "-eL", "timeout", str(seconds), "mgba", "-l", "4",  # Butano logs as WARN
               "-C", "videoSync=0", "-C", "audioSync=0", "-C", "fpsTarget=100000", str(rom)]
    output = []
    try:
        # The ROM never exits: stop the emulator as soon as the run is done.
        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, env=env) as process:
            for line in process.stdout:
                output.append(line)
                if "REPLAY DONE" in line:
                    process.terminate()
                    break
    except FileNotFoundError:
        print("mgba is not installed (it is in the dev container).", file=sys.stderr)
        return 1
    results, done = parse_log("".join(output))
    for level, outcome in results:
        print(f"{outcome:5s} {level}")
    failed = [level for level, outcome in results if outcome == "FAIL"]
    played = [level for level, outcome in results if outcome == "pass"]
    if not done:
        print(f"ERROR the replay ROM did not finish within {seconds} s", file=sys.stderr)
        return 1
    print(f"\nReplays: {len(played)} passed, {len(failed)} failed, "
          f"{len(results) - len(played) - len(failed)} level(s) without a recording.")
    return 1 if failed else 0


def main(argv):
    if argv[:1] == ["import"] and len(argv) == 2:
        return import_sav(argv[1])
    if argv[:1] == ["generate"]:
        return write_header("--check" in argv)
    if argv[:1] == ["layouts"]:
        return check_layouts("--strict" in argv)
    if argv[:1] == ["run"] and len(argv) >= 2:
        seconds = int(argv[argv.index("--seconds") + 1]) if "--seconds" in argv else 120
        return run(argv[1], seconds)
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
