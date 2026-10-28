# Boss music previews

The boss music is not a file of its own: it is the second section of each
world's module in `game/audio/world1..5.xm` (same samples, faster and
darker), written by `tools/Make_boss_music.py`. The game switches to it with
`bn::music::set_position(LevelData::boss_music_order)` when the fight starts.

These WAVs are only for listening (30 s from the boss section, mono, 16 kHz).
They live here and not in `game/audio`, because Butano would build every WAV
there into the ROM as a sound effect.
