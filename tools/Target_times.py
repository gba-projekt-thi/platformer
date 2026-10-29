#!/usr/bin/env python3
"""
Target_times.py

The "Under X seconds" goal of every level (LevelData::target_time_s, shown in
the level select) and the check that it can be met.

The rule, so the targets can be recomputed at any time:

    proof frames = the frames of the level's recording without deaths
                   (game/replays/<LEVEL>.json), which finishes the level.
                   A recording that starts elsewhere (a boss fight tested on
                   its own, "start" in the JSON) is preceded by the walk from
                   the spawn to that start at MAX_SPEED (cfg.h), a lower bound.
    target       = proof frames * FACTOR (1.5), in seconds, rounded up to a
                   whole second.

So the target is always at least the recording's time (beatable by proof),
and a human has to be within a half of the recording to get it. Recordings
are the author's own runs, not tool-assisted, which is what makes this
tough but fair.

Usage (from the repository root):
    python3 tools/Target_times.py            write target_time_s into game/levels/*.json
    python3 tools/Target_times.py --check    every level has a target that is
                                             at least its proof time; exit 1 if not

(After writing, run python3 tools/Generate_levels.py.)
"""

import json
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEVELS_DIR = ROOT / "game/levels"
REPLAY_DIR = ROOT / "game/replays"
FRAMES_PER_SECOND = 60  # goals.h
FACTOR = 1.5


def max_speed(root=ROOT):
    text = (Path(root) / "game/include/cfg.h").read_text()
    return int(re.search(r"\bMAX_SPEED\s*=\s*(\d+)", text).group(1))


def proof_frames(level, replay_dir=REPLAY_DIR, speed=None):
    """Frames in which the level can be finished by the recording without
    deaths, None when it has none."""
    path = Path(replay_dir) / f"{level['name']}.json"
    if not path.exists():
        return None
    recording = json.loads(path.read_text())
    frames = sum(count for _, count in recording["runs"])
    if "start" in recording:
        walk = abs(recording["start"][0] - level["spawn"][0])
        frames += math.ceil(walk / (speed or max_speed()))
    return frames


def target_for(frames):
    """Whole seconds: FACTOR times the proof, rounded up. Integer arithmetic
    (FACTOR = 3/2), so no float edge cases."""
    return -(-(frames * 3) // (2 * FRAMES_PER_SECOND))


def problems(level, replay_dir=REPLAY_DIR, speed=None):
    frames = proof_frames(level, replay_dir, speed)
    target = level.get("target_time_s")
    if frames is None:
        return [f"{level['name']}: no recording without deaths in game/replays"]
    if not isinstance(target, int) or target <= 0:
        return [f"{level['name']}: target_time_s is missing (python3 tools/Target_times.py)"]
    if target * FRAMES_PER_SECOND < frames:
        return [f"{level['name']}: target {target} s is below the recording's "
                f"{frames / FRAMES_PER_SECOND:.1f} s, nobody could beat it"]
    return []


def write_targets(path, targets):
    """The JSON text with target_time_s after each level's music line; a
    line that is there is replaced. Text edit, to keep the file's layout."""
    out, level = [], None
    for line in Path(path).read_text().split("\n"):
        name = re.match(r'^      "name": "(\w+)"', line)
        if name:
            level = name.group(1)
        if re.match(r'^      "target_time_s":', line):
            continue
        out.append(line)
        if level in targets and re.match(r'^      "music": .*,$', line):
            out.append(f'      "target_time_s": {targets[level]},')
    return "\n".join(out)


def main(argv):
    speed = max_speed()
    status = 0
    for path in sorted(LEVELS_DIR.glob("world*.json")):
        levels = json.loads(path.read_text())["levels"]
        if "--check" in argv:
            for level in levels:
                for text in problems(level, speed=speed):
                    print(f"ERROR {text}", file=sys.stderr)
                    status = 1
            continue
        targets = {}
        for level in levels:
            frames = proof_frames(level, speed=speed)
            if frames is None:
                print(f"ERROR {level['name']}: no recording without deaths", file=sys.stderr)
                status = 1
                continue
            targets[level["name"]] = target_for(frames)
        path.write_text(write_targets(path, targets))
        print(f"wrote {path.relative_to(ROOT)}")
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
