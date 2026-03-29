# Audio
_Please note that the following explenations are simplified, for further details please refer to the references._

The game's audio is split into to main components:
- Music: the games soundtrack, longer/looping tracks for each level, .vgm files
- Soundtrack: short sounds that can be played on top of the music, .wave files

## Music
The project's soundtrack is being created with the "Furnace Tracker" program.

In general, the GBA has six audio channels: four legacy channels from the original Gameboy (two pulse wave channels, one noise channel and a wavetable channel --> these are called PSG channels - programmable sound generation channels) and two Direct Sound (PCM) channels that are new to the GBA and can directly output signed 8-bit PCM samples supplied by software at a configurable sample rate

However, through so called software mixing it is possible to mix more than just two channels together in these PCM channels at the cost of CPU load (depending on how many channels were used). For this purpose, Furnace Tracker provides a software mixing driver called MinMod that enables an echo effect along with up to 16 channels.

With Furnace Tracker, music can be exported with the .vgm file extension, which is compatible with butano. It is important to select version 1.61 when exporting.

After exporting, put the file into the src/dmg_audio folder and build the project. This will create two new files in the build folder:
- bn_dmg_music_items_your_song_name.h
- your_song_name_bn_dmg.c

To play the added soundtrack, just include the corresponding header file you just created and play the music using

```
bn::dmg_music_items::your_song_name.play();
```

## Sound Effects
To add a sound effect, just put the desired .wav file into the audio folder, build the project (which adds to bn_sound_items.h), include bn_sound_items.h and play the sfx using 
```
bn::sound_items::your_sfx_name.play();
```


## References
- [https://jsgroth.dev/blog/posts/gba-audio/] 
This article explains the GBA's audio channels and gives some good examples on how the PCM and PSG channels are split up
- [https://gvaliente.github.io/butano/import.html#import_dmg_music] 
Butano documentation on how to export and add VMG music
- [https://gvaliente.github.io/butano/import.html#import_sound] 
Butano documentation on how to add sound effects
- [https://tildearrow.org/furnace/doc/latest/manual.pdf] The Furnace Tracker manual, providing more details on the software and MinMod
- [https://elirulz.net/guides/gba/] A guide that further explains MinMod and the best settings to use
