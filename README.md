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
cmake --build -j $(nproc)
```

To enable tests add `-DBUILD_TESTS=ON` to `cmake -DBUILD_TOOLS=ON ..`
Tests can be launched by:

```
ctest --output-on-failure
```

Executables can be located in the build directory

With tests enabled, `test/manual/startAtLevel1Car` is also built. It starts
the game at the stage-1 bonus drive, as if the first boss had just been
beaten, so the car scene can be tried without playing up to it. Like the game,
run it from the directory that holds `assets`.

# FrankoExtract

it's a tool to extract graphics/sounds/music/levels from original franko game data.

Only Version 1.0 is supported right now.

Usage:

```
./frankoExtract -i {game_data_directory} -o {output_directory} [-e {game_executable}]
```

The ending credits are read from the compiled game program, the `game` file
the original installer puts next to the data files. It is picked up
automatically when it sits in the game data directory; otherwise pass it with
`-e`. Without it the ending credits are not extracted.

Game data directory must contain files:

```
0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007, 0008, 0009, 000A, 000B, 000C, 000D, 000E, 000F, 0010, 0011, 0012, 0013, 0014, 0015, 0034, 0035, 0036, 0037, 0038, 0094, 0095, 0096, 00C6, 00C7, 00C8, 00F6, 00F7, 00F8, 00F9, 00FA, 00FB, 00FC, 00FD, 00FE, 00FF, 0137, 0138, 0139, 013A, 013B, 013C, 013D, 013E, 013F, 0140, 0141, 0142, 0143, 0144, 0145, 014A, 014B, 014C, 014D, 014E, 014F, 0154, 0259, 025A, 025B, 025C, 025D, 025E, 025F, 0261, 0262, 0263, 0384, 0385, 0386, 0387, 0388, 0389, 038A, 038B, 038C, 03B6, 03B7, 03B8, 03B9, 03BA, 03BB, 03BC, 03BD, 03BE, 03BF, 03C0, 03C1, 03C2, 03C3
```

Data will be extracted as:

- Graphics as bitmap files (.bmp)
- Sounds as wave files (.wav)
- Music as ScreamTracker3 modules (.s3m)
- Level scripts (enemy waves) as JSON files (.json)
- Copy protection code cards as a JSON file (0384_codecards.json)
- Ending credits as a JSON file (credits.json), from the game executable
