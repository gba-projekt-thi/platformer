#!/usr/bin/env python3
"""
Generate_dash_sound.py

Writes the movement sound effects as Butano/maxmod-ready WAVs (filtered
noise sweeps, 8-bit unsigned PCM, mono, 11025 Hz):

    dash.wav   a "whoosh" sweeping down in pitch, ~0.2 s
    pogo.wav   the pogo strike's short "swish" sweeping up, ~0.12 s

Fully deterministic (fixed noise seed), pure standard library. Usage:

    python3 tools/Generate_dash_sound.py game/audio

The names become bn::sound_items::dash / pogo. Replace a file with a
hand-made sample at any time; the game only relies on the name.
"""

import math
import sys
import wave
from pathlib import Path

RATE = 11025
PEAK = 0.85
# name: (seconds, cutoff at the start, cutoff at the end, decay)
SOUNDS = {
    "dash": (0.20, 3500.0, 500.0, 5.0),
    "pogo": (0.12, 1200.0, 4500.0, 4.0),
}


def samples(name="dash"):
    duration, start, end, decay_rate = SOUNDS[name]
    n = int(RATE * duration)
    state = 0x1234ABCD
    lp1 = lp2 = 0.0
    raw = []
    for i in range(n):
        # xorshift32 noise in [-1, 1]
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= state >> 17
        state ^= (state << 5) & 0xFFFFFFFF
        noise = (state / 0x7FFFFFFF) - 1.0

        t = i / n
        cutoff = start * (1.0 - t) + end * t
        a = 1.0 - math.exp(-2.0 * math.pi * cutoff / RATE)
        lp1 += a * (noise - lp1)
        lp2 += a * (lp1 - lp2)
        band = lp1 - lp2

        attack = min(1.0, i / (0.012 * RATE))
        decay = math.exp(-decay_rate * t)
        raw.append(band * attack * decay)

    peak = max(abs(s) for s in raw) or 1.0
    return [int(round(128 + 127 * PEAK * s / peak)) for s in raw]


def write(path, name="dash"):
    data = bytes(max(0, min(255, s)) for s in samples(name))
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(1)
        wav.setframerate(RATE)
        wav.writeframes(data)


def main(argv):
    out_dir = Path(argv[0]) if argv else Path(".")
    out_dir.mkdir(parents=True, exist_ok=True)
    for name in SOUNDS:
        write(out_dir / f"{name}.wav", name)
        print(f"wrote {out_dir / (name + '.wav')}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
