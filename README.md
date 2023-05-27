# DivFX

Path of Exile's divination cards have a small piece of art in the upper half of the art that illustrates the card. Some specific cards can have an animated background behind its card art, influenced by the Shaper, the Elder or one of the four Conquerors of the Atlas.

Divination cards are composed from a black or animated background, per-card static art, the card frame/body and the text of the card.

## What it does

This tool utilizes game assets to render these animations into looping video files suitable for accurately drawing divination cards.

Note: all paths and animation sources are hardcoded so mild modification is needed to run this on Someone Else's Computer or when new influence combinations ship.

The background is composed as follows:

* draw an opaque black background;
* draw each animated layer on top, blending it with the previous contents.

Each divination card has a list of layers to apply, existing combinations are from zero up to two different influences, as indicated by the third field of `DivinationCardArt.dat64`.

The influences are in order:

* 0: Shaper
* 1: Elder
* 2: Crusader
* 3: Redeemer
* 4: Hunter
* 5: Warlord

## Building it

```
git clone --recursive
cmake -B build-divfx -S divfx --toolchain "vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build-divfx --config RelWithDebInfo
```

`gen.bat` contains example invocations of `ffmpeg` to generate the current set of video files from a subdirectory `raw` with the output from the program, where constructs like `0_1` represents an animation with the Shaper (0) below the Elder (1).