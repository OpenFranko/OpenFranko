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

In the game, Alt+Enter switches between a window and fullscreen. In
fullscreen the game switches the screen to 50 Hz (60 Hz in NTSC mode) when the
screen offers that rate, like many TVs and external monitors do, so it runs
perfectly smoothly; otherwise it keeps the desktop's rate.

With tests enabled, three launchers are also built in `test/manual`, so late
scenes can be tried without playing up to them. `startAtLevel1Car` starts the
game at the stage-1 bonus drive, as if the first boss had just been beaten.
`startAtEnding` starts it at the ending, as if the third boss had just been
beaten. `startAtGameOver` starts it at the game over graveyard, as if the last
life had just been lost on stage 1. Like the game, run them from the directory
that holds `assets`.

The AMAL animation programs of the street scenes, in
`src/engine/street/actors/Actors.cpp`, are compiled ahead of time into
`src/engine/street/actors/compiled/CompiledActors.cpp`, so the game never parses
AMAL while it plays. After changing a program, regenerate that file with the
`compileActorPrograms` tool, built with the tools, from the build directory:

```
./tools/compiler/compileActorPrograms -o ../src/engine/street/actors/compiled/CompiledActors.cpp
```

`compiledActorsTest` fails while the file is out of date.

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

## DOS (DJGPP)

The DOS version is cross-compiled with DJGPP and uses Allegro 4 instead of
SDL2. The tools and tests are not built for DOS; extract the game data with a
Linux or Windows build of FrankoExtract.

`build-dos.sh` needs no DJGPP installed: it downloads DJGPP (GCC 12 for
`i586-pc-msdosdjgpp`, from [build-djgpp](https://github.com/andrewwutw/build-djgpp))
and DJGPP's Allegro 4.2.2 into `build-dos/djgpp`, builds libxmp for them, builds
the game in `build-dos` and puts a runnable copy in `build-dos/game`:

```
./build-dos.sh --assets <assets_dir>
dosbox -conf build-dos/game/dosbox.conf
```

The downloaded DJGPP's `ar` needs flex's library `libfl.so.2`; on Debian and
Ubuntu, install it with `sudo apt install libfl2`.

Options starting with `-D` are passed to CMake, e.g.
`./build-dos.sh -DSKIP_COPY_PROTECTION=ON`.

`build-dos/game` holds `franko.exe`, `CWSDPMI.EXE` (the DPMI host), the assets
packed into `assets.tar` and a `dosbox.conf` for DOSBox, DOSBox Staging and
DOSBox-X (`flatpak run com.dosbox_x.DOSBox-X -conf
"$PWD/build-dos/game/dosbox.conf"`; the flatpak starts in the home directory,
so the path has to be absolute). DOS has no long file names, so the game reads
its assets from the archive; every build does that when `assets.tar` sits next
to it instead of `assets`. On a real PC, copy the first three files into one
directory and run `franko`. It needs a VGA card and, for sound, a Sound Blaster
compatible card; without one it says so while it loads and plays silently.
Ctrl+C or Ctrl+Break quits it.

The game shows the Amiga picture pixel for pixel in a 376x282 256-colour VGA
mode (Mode X) that, like a PAL Amiga, refreshes about 50 times a second; high
resolution screens are shown at half their width. It needs a 486 with a
floating point unit and 8 MB of memory, and its logic is tied to the frame
rate, so a slower PC plays it in slow motion. In DOSBox-X with its CPU speed
presets, Level 1 keeps its 50 frames a second on a 486DX2-66 or faster, apart
from short pauses while it loads scenery. The music keeps playing through
them, and nearly every frame is drawn during the vertical blank, so the picture
rarely tears. A 486DX-33 runs it at about 35 frames a second; there the music
only stays smooth with 16 MB of memory. Before it starts, the game measures
the PC's speed and memory, and on a slower PC or with less memory it says so
and asks whether to start anyway.

To build with an installed DJGPP instead, like the AUR packages `djgpp-gcc`,
`djgpp-allegro4` and `djgpp-cmake`, build libxmp with its CMake wrapper and
install it into the DJGPP directory, then build the game the same way:

```
curl -LO https://github.com/libxmp/libxmp/releases/download/libxmp-4.7.3/libxmp-4.7.3.tar.gz
tar xzf libxmp-4.7.3.tar.gz
cd libxmp-4.7.3
i686-pc-msdosdjgpp-cmake -B build -DBUILD_SHARED=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-march=i586
cmake --build build -j $(nproc)
sudo cmake --install build
cd ../OpenFranko
mkdir build-djgpp && cd build-djgpp
i686-pc-msdosdjgpp-cmake ..
cmake --build . -j $(nproc)
```

The AUR packages build the C++ library and Allegro for the Pentium Pro, so that
`src/franko.exe` needs a Pentium Pro or newer and does not run in DOSBox or
DOSBox Staging (in DOSBox-X, set `cputype=pentium_ii`). The game's own code and
libxmp avoid Pentium Pro instructions (`-march=i586`), as DOSBox-X's fast CPU
core mis-emulates the Pentium Pro floating point comparisons.

## Atari Jaguar

The Jaguar version runs the game on the console's 68000. TOM draws the
picture: its Object Processor shows the screen layers, its blitter draws the
sprites and scenery, and its GPU changes the palette between lines like the
Amiga's copper. JERRY's DSP mixes the music and the sound effects at about
22 kHz, with the Amiga's low-pass filter; the 68000 reads the S3M modules and
tells the DSP what each voice plays.

It needs CMake, curl, git, make and a C++ compiler for the computer you build
on. `build-jaguar.sh` downloads an `m68k-elf` GCC 15.2 with C++ support and
newlib from [toolchain68k](https://github.com/haarer/toolchain68k) into
`build-jaguar/toolchain` (about 320 MB, checked against its SHA-256). It runs
on x86-64 Linux and uses the GMP, MPFR, MPC, isl and zstd libraries that a
native GCC needs too. To use an installed toolchain instead, set
`JAGUAR_TOOLCHAIN_PREFIX`, e.g. `JAGUAR_TOOLCHAIN_PREFIX=m68k-elf-` with the
AUR packages `m68k-elf-gcc` and `m68k-elf-newlib` on Arch. It also fetches the
[Jaguar SDK](https://github.com/cubanismo/jaguar-sdk) at a fixed commit into
`build-jaguar/jaguar-sdk` and builds its `rmac` assembler and `jagcrypt`. The
script then builds the game into `build-jaguar`, packs the extracted game
data with it and writes a 4 MB cartridge image,
`build-jaguar/game/franko.j64`:

```
./build-jaguar.sh --assets <assets_dir>
```

To use an SDK whose `maketools.sh` has already built its tools, pass
`--sdk <jaguar_sdk_dir>`; nothing is fetched either when `rmac` is on the
`PATH`. Options starting with `-D` are passed to CMake, e.g.
`-DSKIP_COPY_PROTECTION=ON`. The image plays the version of the game that was
extracted into `<assets_dir>`, 1.0 or 1.2. When the SDK's `jagcrypt` is found,
the image gets the encrypted boot block that a console checks before it starts
a cartridge, so it runs on a console from a flash cartridge as well as in
emulators like BigPEmu; without `jagcrypt` it only runs in emulators.

The joypad plays like the Amiga joystick: the pad moves Franko and A, B and C
are fire (Space). The keypad stands in for the keyboard: 1 to 4 are F1 to F4, 9
is F9, `*` and Pause are Esc, `#` is Del, and Option is the mouse button. When
the game asks for a name for the high score table or for a letter from the code
card, a keyboard opens at the top of the screen: left and right move to the
next letter, up and down jump five, A types the letter, B deletes one, and C is
Return. For the code card a smaller keyboard opens over the "Podaj kod!"
button, so the grid and the colours stay visible, and it starts on the blank,
so pressing fire does not answer by accident. Outside the game itself, 0 shows
or hides that keyboard. Option and 0 together show how many frames the game
keeps up with and how much memory it uses; building with
`-DJAGUAR_DEBUG_OVERLAY=ON` shows that from the start.

The console sets the frame rate: 50 Hz on a PAL console, like a PAL Amiga, and
60 Hz on an NTSC console, where the game starts in its NTSC mode and runs
faster, as it did on an NTSC Amiga. The game's PAL and NTSC keys switch its
screen layout, not the console's video standard; an NTSC TV shows 241 of the
256 lines of the PAL layout. The high score table is kept in the cartridge's
EEPROM; with an empty or damaged EEPROM the game starts with its usual table.

The 68000 does not always keep up. In BigPEmu, a fight on level 1 updates about
95% of the frames on a PAL console and about 80% on an NTSC console, so busy
moments play a little slower; the music keeps its tempo.

# Releases

GitHub Actions (`.github/workflows/ci-cd.yml`) builds OpenFranko for Linux,
Windows, DOS and the Atari Jaguar and runs the tests on Linux and Windows for
every pull request and every push to `main`, and keeps the packages it makes
on the run's summary page. Pushing a tag that starts with `v` builds them the
same way and publishes them as a GitHub release:

```
git tag v1.0
git push origin v1.0
```

A tag with a `-` in it, like `v1.0-rc1`, makes a pre-release. If the release
already exists, e.g. because it was written on GitHub first, the packages are
added to it.

The packages hold no game data. Extract it with their `frankoExtract` as
described below, and run the game from the directory that holds `assets` or
`assets.tar`:

- `OpenFranko-linux-x86_64.tar.gz` holds `OpenFranko` and `frankoExtract`.
  They are built on Ubuntu 24.04, so they need it or a newer distribution, and
  the SDL2 and libxmp libraries (`libsdl2-2.0-0` and `libxmp4` on Debian and
  Ubuntu).
- `OpenFranko-windows-x86_64.zip` holds `OpenFranko.exe` and
  `frankoExtract.exe`, which need no DLLs.
- `OpenFranko-dos.zip` holds `franko.exe` and `CWSDPMI.EXE`. Extract the game
  data with the Linux or Windows package, pack the `assets` directory with
  `tar --format=ustar -cf assets.tar assets` (Windows 10 and newer have `tar`
  too) and put `assets.tar` next to `franko.exe`. CWSDPMI is by Charles W
  Sandmann, see `cwsdpmi.doc`; its source code is at
  <https://www.delorie.com/pub/djgpp/current/v2misc/csdpmi7s.zip>.

A Jaguar cartridge has to hold the game data, so releases have no Jaguar
package; build one with `build-jaguar.sh --assets`. Each run keeps the
Jaguar program it built, `franko.elf` and `franko.bin`, in
`OpenFranko-jaguar-engine.zip` on its summary page, and also compiles the
on-target test programs, `jaguarSelfTest` and `jaguarProfileGame`.

# FrankoExtract

it's a tool to extract graphics/sounds/music/levels from original franko game data.

Versions 1.0 and 1.2 are supported. The 1.2 files are extracted under their
own names, and the game runs on either extraction: when `assets` holds the 1.2
files (it has `p0/p0.bmp`), OpenFranko plays version 1.2, with its spider logo,
advert slideshow, intro texts, cheats and other changes. The 1.2 intro texts
come from the game executable, like the ending credits; without `intro.json`
the intro skips them. Before it starts, the game checks that the extracted
files it needs are all there; if some are missing, it names them and quits.

Usage:

```
./frankoExtract -i {game_data_directory} -o {output_directory} [-e {game_executable}]
```

The ending credits are read from the compiled game program, the `game` file
the original installer puts next to the data files. It is picked up
automatically when it sits in the game data directory; otherwise pass it with
`-e`. The game needs the credits for its ending, so without the `game` file
frankoExtract stops, as it does when a data file is missing.

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
