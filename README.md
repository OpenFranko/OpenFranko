# OpenFranko

Open source C++ implementation of Franko: The Crazy Revenge engine

Motivation:
Franko: The Crazy Revenge is a Polish cult classic video game from the Amiga computer. The goal of this project is to make a implementation that will run on any computer/operating system, and will be easy to modify.

THIS PROJECT IS WIP, SOME PARTS OF THE CODEBASE WERE DEVELOPED WITH ASSISTANCE OF LLM

# Build instructions

## Linux

Install dependencies:

Debian/Ubuntu

```
sudo apt update
sudo apt upgrade
sudo apt install build-essential cmake git libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libxmp-dev catch2
```

Arch

```
sudo pacman -Syu
sudo pacman -S base-devel cmake git sdl2 sdl2_image sdl2_mixer libxmp catch2
```

Compilation:

```
git clone https://github.com/OpenFranko/OpenFranko.git
cd OpenFranko
mkdir build && cd build
cmake -DBUILD_TOOLS=ON .. 
cmake --build . -j $(nproc)
```

To enable tests add `-DBUILD_TESTS=ON` to `cmake -DBUILD_TOOLS=ON ..`
Tests can be launched by:

```
ctest --output-on-failure
```

Executables can be located in the build directory

With tests enabled, three launchers are also built in `test/manual`, so late
scenes can be tried without playing up to them. `startAtLevel1Car` starts the
game at the stage-1 bonus drive, as if the first boss had just been beaten.
`startAtEnding` starts it at the ending, as if the third boss had just been
beaten. `startAtGameOver` starts it at the game over graveyard, as if the last
life had just been lost on stage 1. Like the game, run them from the directory
that holds `assets`.

## Windows (MSYS2)

Install [MSYS2](https://www.msys2.org/) and open the `MSYS2 UCRT64` terminal.

Install dependencies (if `pacman -Syu` closes the terminal, open it again and
repeat the command):

```
pacman -Syu
pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-libxmp mingw-w64-ucrt-x86_64-catch
```

Compilation:

```
git clone https://github.com/OpenFranko/OpenFranko.git
cd OpenFranko
mkdir build && cd build
cmake -G Ninja -DBUILD_TOOLS=ON ..
cmake --build . -j $(nproc)
```

Tests are enabled and launched the same way as on Linux.

The executables are linked statically, so they also run outside of MSYS2 without
any extra DLLs. Like on Linux, the game has to be started from the directory
that holds `assets`, e.g. copy `build/src/OpenFranko.exe` next to `assets` and
double-click it.

# FrankoExtract

it's a tool to extract graphics/sounds/music/levels from original franko game data.

Versions 1.0 and 1.2 are supported. The 1.2 files are extracted under their
own names, and the game runs on either extraction: when `assets` holds the 1.2
files (it has `p0/p0.bmp`), OpenFranko plays version 1.2, with its spider logo,
advert slideshow, intro texts, cheats and other changes. The 1.2 intro texts
come from the game executable, like the ending credits; without `intro.json`
the intro skips them.

Usage:

```
./frankoExtract -i {game_data_directory} -o {output_directory} [-e {game_executable}]
```

The ending credits are read from the compiled game program, the `game` file
the original installer puts next to the data files. It is picked up
automatically when it sits in the game data directory; otherwise pass it with
`-e`. Without it the ending credits are not extracted.

Version 1.0 game data directory must contain files:

```
0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007, 0008, 0009, 000A, 000B, 000C, 000D, 000E, 000F, 0010, 0011, 0012, 0013, 0014, 0015, 0034, 0035, 0036, 0037, 0038, 0094, 0095, 0096, 00C6, 00C7, 00C8, 00F6, 00F7, 00F8, 00F9, 00FA, 00FB, 00FC, 00FD, 00FE, 00FF, 0137, 0138, 0139, 013A, 013B, 013C, 013D, 013E, 013F, 0140, 0141, 0142, 0143, 0144, 0145, 014A, 014B, 014C, 014D, 014E, 014F, 0154, 0259, 025A, 025B, 025C, 025D, 025E, 025F, 0261, 0262, 0263, 0384, 0385, 0386, 0387, 0388, 0389, 038A, 038B, 038C, 03B6, 03B7, 03B8, 03B9, 03BA, 03BB, 03BC, 03BD, 03BE, 03BF, 03C0, 03C1, 03C2, 03C3
```

Version 1.2 game data directory must contain files:

```
m1-m7, m9, m10, m11, p0-p8, p50-p62, p80-p85, s0-s21, s50, s52-s56, s148, s149, s150, s198, s199, s200, s246-s255, t11-t25, t30-t35, t40
```

A file named like one of the 1.2 files above is read as a 1.2 file, whether it
is in the directory or given alone to `frankoExtract` or to one of the other
extraction tools; any other file is read as a 1.0 file.

Data will be extracted as:

- Graphics as bitmap files (.bmp)
- Sounds as wave files (.wav)
- Music as ScreamTracker3 modules (.s3m)
- Level scripts (enemy waves) as JSON files (.json)
- Copy protection code cards as a JSON file (0384_codecards.json, p0_codecards.json for 1.2)
- Ending credits as a JSON file (credits.json), from the game executable
- Intro texts of version 1.2 as a JSON file (intro.json), from the game executable
