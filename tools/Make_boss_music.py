#!/usr/bin/env python3
"""
Make_boss_music.py

Derives each world's boss track from the world's own XM module, so it uses
exactly the same samples and melodic material, only more intense. The boss
track lives INSIDE the world module as a second song section:

    orders  0 .. N-1   the world theme (unchanged; its last row jumps back
                       to the restart order with a Bxx effect, which is what
                       the module did anyway)
    orders  N .. M-1   the boss section: copies of the module's most energetic
                       patterns, played at a faster tempo (Fxx on its first
                       row) with a driving layer on added channels (a kick on
                       every beat, an off-beat hat, and the bass line
                       re-struck in eighth notes wherever it used to sustain),
                       looping on itself with Bxx

maxmod has no cheap way to share samples between modules (every module ships
its own copy, ~1 MB per world), but a second section costs only pattern data
(a few KB). The game starts the boss section with bn::music::set_position()
(LevelData::boss_music_order, checked against the module by this script).

    python3 tools/Make_boss_music.py            rewrite game/audio/world*.xm
    python3 tools/Make_boss_music.py --check    fail if a module / level JSON
                                                is not what this script makes

Running it again is harmless: the boss section of an already converted module
is stripped first, so the result only depends on the world theme.
"""

import argparse
import json
import struct
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import xmfile  # noqa: E402
from xmfile import Note, Pattern  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
AUDIO = ROOT / "game" / "audio"
LEVELS = ROOT / "game" / "levels"

FX_JUMP = 0x0B  # Bxx: jump to order xx
FX_EXTENDED = 0x0E  # Exy
FX_TEMPO = 0x0F  # Fxx: xx >= 32 sets the BPM
KEY_OFF = 97
MIN_CHANNELS = 8

BPM_FACTOR = 1.25  # boss tempo relative to the theme
KEEP_SHARE = 0.55  # share of the theme's patterns that goes into the boss loop
MIN_KEEP = 4
KICK_SHARE = 1.0  # volume of the added kick / hat / bass pulse, relative to
HAT_SHARE = 0.5  # the instrument's own default volume (a quieter hat is darker)
PULSE_SHARE = 0.7
DRONE_SHARE = 0.35  # the low pedal tone under every pattern
STAB_SHARE = 0.25  # the tritone stab before the loop point
DRONE_LOW = 25  # the pedal lives between C-2 and B-2
PULSE_FLOOR = 37  # bass pulses above C-3 are played an octave lower
DEFAULT_KICK_NOTE = 49  # C-4, when the theme never plays the instrument

# Instruments of the world's own percussion that make up the driving layer
# (1-based, see the module). None = no such layer.
PRESETS = {
    "world1": {"kick": 5, "hat": 4, "drone": 1},  # Bassdrum, Tambourin; Wave
    "world2": {"kick": 3, "hat": 10, "drone": 6},  # kick, Ting; Bass
    # No percussion in the theme: the low Bass as a heartbeat (cut after 4
    # ticks), the pitched-up "Bass short" as a tick.
    "world3": {"kick": 6, "kick_cut": 4, "hat": 7, "drone": 6, "keep_pulse": True},
    "world4": {"kick": 6, "hat": 5, "drone": 1},  # Drums, Maracas; Guitar
    "world5": {"hat": 4, "hat_cut": 3, "drone": 1},  # Noise, cut short as the theme does (EC3)
}


# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------


def _notes(mod, pattern):
    return [(r, c, n) for r, row in enumerate(pattern.rows)
            for c, n in enumerate(row) if n.note and n.note != KEY_OFF]


def energy(mod, pattern):
    """Notes struck in the pattern: the measure of how busy it is."""
    return len(_notes(mod, pattern))


def instrument_volume(mod, inst):
    raw = mod.instruments[inst - 1]
    (header,) = struct.unpack_from("<I", raw, 0)
    return raw[header + 12] if len(raw) > header + 12 else 64


def boss_start(mod):
    """Order of the first boss pattern, or None if there is no boss section.

    The theme's last pattern ends in a Bxx jump; what follows is the boss.
    """
    for o, pi in enumerate(mod.order):
        last = mod.patterns[pi].rows[-1]
        if any(n.fx == FX_JUMP for n in last):
            return o + 1 if o + 1 < len(mod.order) else None
    return None


def _trim_channels(mod):
    used = 0
    for p in mod.patterns:
        for row in p.rows:
            for c, n in enumerate(row):
                if not n.empty():
                    used = max(used, c + 1)
    mod.channels = max(MIN_CHANNELS, used)
    for p in mod.patterns:
        p.rows = [row[: mod.channels] for row in p.rows]


def strip(mod):
    """A copy of the module with its boss section removed (the world theme)."""
    out = xmfile.parse(xmfile.write(mod))
    start = boss_start(out)
    if start is None:
        return out
    out.order = out.order[:start]
    seen = sorted(set(out.order))
    remap = {old: new for new, old in enumerate(seen)}
    out.patterns = [out.patterns[pi] for pi in seen]
    out.order = [remap[pi] for pi in out.order]
    last = out.patterns[out.order[-1]].rows[-1]
    for n in last:
        if n.fx == FX_JUMP:
            n.fx = n.param = 0
    first = out.patterns[out.order[0]].rows[0]
    for n in first:
        if n.fx == FX_TEMPO and n.param >= 32:
            n.fx = n.param = 0
    _trim_channels(out)
    return out


def _free_channels(mod):
    return [c for c in range(mod.channels)
            if all(row[c].empty() for p in mod.patterns for row in p.rows)]


def _add_channels(mod, count):
    for p in mod.patterns:
        for row in p.rows:
            row.extend(Note() for _ in range(count))
    mod.channels += count


def _modal_note(mod, inst, default):
    counts = Counter(n.note for p in mod.patterns for row in p.rows
                     for n in row if n.inst == inst and 0 < n.note < KEY_OFF)
    return counts.most_common(1)[0][0] if counts else default


def _bass_channel(mod, patterns, ignore):
    """Channel with the lowest notes (average) among those that play enough."""
    best, best_pitch = None, None
    for c in range(mod.channels):
        pitches = [n.note for p in patterns for row in p.rows
                   for n in [row[c]] if 0 < n.note < KEY_OFF and n.inst not in ignore]
        if len(pitches) < 8:
            continue
        pitch = sum(pitches) / len(pitches)
        if best_pitch is None or pitch < best_pitch:
            best, best_pitch = c, pitch
    return best


def _cut(ticks):
    """(fx, param) of a note cut after `ticks` ticks (ECx), or none."""
    return (FX_EXTENDED, 0xC0 | ticks) if ticks else (0, 0)


def _vol_column(mod, inst, share):
    return 0x10 + max(1, min(64, round(instrument_volume(mod, inst) * share)))


def _add_drive(mod, pattern, layers, bass, preset, kick_note, hat_note):
    kick, hat = preset.get("kick"), preset.get("hat")
    pulse_ch, kick_ch, hat_ch = layers
    rows = pattern.rows
    if kick:
        for r in range(0, len(rows), 4):
            rows[r][kick_ch] = Note(kick_note, kick, _vol_column(mod, kick, KICK_SHARE),
                                    *_cut(preset.get("kick_cut")))
    if hat:
        for r in range(2, len(rows), 4):
            rows[r][hat_ch] = Note(hat_note, hat, _vol_column(mod, hat, HAT_SHARE),
                                   *_cut(preset.get("hat_cut")))
    if bass is None:
        return
    events = [r for r in range(len(rows)) if rows[r][bass].note]
    for i, r0 in enumerate(events):
        note = rows[r0][bass]
        if note.note == KEY_OFF or not note.inst:
            continue
        end = events[i + 1] if i + 1 < len(events) else len(rows)
        for r in range(r0 + 2, end - 1, 2):
            # An octave lower (world3's pitched-up tick stays as it is).
            low = note.note > PULSE_FLOOR and not preset.get("keep_pulse")
            rows[r][pulse_ch] = Note(
                note.note - 12 if low else note.note, note.inst, _vol_column(mod, note.inst, PULSE_SHARE))


def _root(rows, bass, ignore):
    """Pitch class (0..11) of the bass channel's most common note, or None."""
    counts = Counter((r[bass].note - 1) % 12 for r in rows
                     if bass is not None and 0 < r[bass].note < KEY_OFF
                     and r[bass].inst not in ignore)
    return counts.most_common(1)[0][0] if counts else None


def _add_menace(mod, pattern, drone_ch, bass, preset, ignore, last):
    """A pedal tone on the pattern's root, far below the melody; the last
    pattern adds a tritone stab on the way back to the loop start."""
    inst = preset.get("drone")
    root = _root(pattern.rows, bass, ignore)
    if not inst or root is None:
        return
    note = DRONE_LOW + (root + 1 - DRONE_LOW) % 12  # the root in C-2 .. B-2
    rows = pattern.rows
    rows[0][drone_ch] = Note(note, inst, _vol_column(mod, inst, DRONE_SHARE))
    if last:
        rows[len(rows) * 3 // 4][drone_ch] = Note(
            note + 6, inst, _vol_column(mod, inst, STAB_SHARE))


def build(mod, preset, bpm_factor=BPM_FACTOR):
    """The module with a boss section appended (see the module docstring)."""
    out = strip(mod)
    theme_orders = list(out.order)
    base_bpm = out.bpm
    boss_bpm = min(255, round(base_bpm * bpm_factor))

    # Free (or new) channels for the drive layer; the first also carries the
    # tempo / jump effects, so the theme's own channels stay untouched.
    free = _free_channels(out)
    if len(free) < 4:
        add = 4 - len(free)
        first_new = out.channels
        _add_channels(out, add)
        free += list(range(first_new, first_new + add))
    drone_ch = free[-4]
    layers = free[-3:]
    control = layers[0]

    # Most energetic patterns, in song order.
    ranked = sorted(set(theme_orders),
                    key=lambda pi: (-energy(out, out.patterns[pi]), theme_orders.index(pi)))
    keep = max(MIN_KEEP, round(len(set(theme_orders)) * KEEP_SHARE))
    keep = min(keep, len(set(theme_orders)))
    chosen = sorted(ranked[:keep], key=theme_orders.index)

    kick, hat = preset.get("kick"), preset.get("hat")
    ignore = {i for i in (kick, hat) if i}
    chosen_patterns = [out.patterns[pi] for pi in chosen]
    bass = _bass_channel(out, chosen_patterns, ignore)
    kick_note = _modal_note(out, kick, DEFAULT_KICK_NOTE) if kick else 0
    hat_note = _modal_note(out, hat, DEFAULT_KICK_NOTE) if hat else 0

    boss_first = len(theme_orders)
    for i, pi in enumerate(chosen):
        copy = Pattern([[Note(**vars(n)) for n in row] for row in out.patterns[pi].rows])
        _add_drive(out, copy, layers, bass, preset, kick_note, hat_note)
        _add_menace(out, copy, drone_ch, bass, preset, ignore, i == len(chosen) - 1)
        out.patterns.append(copy)
        out.order.append(len(out.patterns) - 1)

    # Tempo: the boss section is faster, the theme's first row restores it.
    out.patterns[out.order[boss_first]].rows[0][control].fx = FX_TEMPO
    out.patterns[out.order[boss_first]].rows[0][control].param = boss_bpm
    out.patterns[out.order[0]].rows[0][control].fx = FX_TEMPO
    out.patterns[out.order[0]].rows[0][control].param = base_bpm
    # Loops: the theme back to its restart order, the boss onto itself.
    theme_last = out.patterns[out.order[len(theme_orders) - 1]]
    theme_last.rows[-1][control].fx = FX_JUMP
    theme_last.rows[-1][control].param = out.restart
    boss_last = out.patterns[out.order[-1]]
    boss_last.rows[-1][control].fx = FX_JUMP
    boss_last.rows[-1][control].param = boss_first
    return out


# ----------------------------------------------------------------------------
# Files
# ----------------------------------------------------------------------------


def convert_file(path, preset):
    mod = xmfile.parse(path.read_bytes())
    return xmfile.write(build(mod, preset))


def json_boss_orders():
    """{music name: [(level name, boss_music_order)]} from game/levels/*.json."""
    found = {}
    for path in sorted(LEVELS.glob("world*.json")):
        data = json.loads(path.read_text())
        for level in data["levels"]:
            if "boss_music_order" in level:
                found.setdefault(level["music"], []).append(
                    (level["name"], level["boss_music_order"]))
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--check", action="store_true",
                        help="only verify the modules and the level JSON")
    args = parser.parse_args(argv)

    problems = []
    orders = json_boss_orders()
    for name, preset in PRESETS.items():
        path = AUDIO / f"{name}.xm"
        if not path.exists():
            continue
        new = convert_file(path, preset)
        if args.check:
            if new != path.read_bytes():
                problems.append(f"{path.name}: run tools/Make_boss_music.py")
        else:
            path.write_bytes(new)
            print(f"{path.name}: boss section from order "
                  f"{boss_start(xmfile.parse(new))}, {len(new)} bytes")
        start = boss_start(xmfile.parse(new))
        for level, order in orders.get(name, []):
            if order != start:
                problems.append(
                    f"{level}: boss_music_order {order}, {path.name} has {start}")
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
