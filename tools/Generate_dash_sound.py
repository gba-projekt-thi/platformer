#!/usr/bin/env python3
"""
Generate_dash_sound.py

Writes the dash sound effect (a short filtered-noise "whoosh" that sweeps
down in pitch) as a Butano/maxmod-ready WAV:

    dash.wav   8-bit unsigned PCM, mono, 11025 Hz, ~0.2 s

Fully deterministic (fixed noise seed), pure standard library. Usage:

    python3 tools/Generate_dash_sound.py game/audio

The name becomes bn::sound_items::dash. Replace the file with a hand-made
sample at any time; the game only relies on the name.
"""

import math
import sys
import wave
from pathlib import Path

RATE = 11025
DURATION = 0.20
PEAK = 0.85


def samples():
    n = int(RATE * DURATION)
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
        cutoff = 3500.0 * (1.0 - t) + 500.0 * t  # sweep down
        a = 1.0 - math.exp(-2.0 * math.pi * cutoff / RATE)
        lp1 += a * (noise - lp1)
        lp2 += a * (lp1 - lp2)
        band = lp1 - lp2

        attack = min(1.0, i / (0.012 * RATE))
        decay = math.exp(-5.0 * t)
        raw.append(band * attack * decay)

    peak = max(abs(s) for s in raw) or 1.0
    return [int(round(128 + 127 * PEAK * s / peak)) for s in raw]


def write(path):
    data = bytes(max(0, min(255, s)) for s in samples())
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(1)
        wav.setframerate(RATE)
        wav.writeframes(data)


def main(argv):
    out_dir = Path(argv[0]) if argv else Path(".")
    out_dir.mkdir(parents=True, exist_ok=True)
    write(out_dir / "dash.wav")
    print(f"wrote {out_dir / 'dash.wav'}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
