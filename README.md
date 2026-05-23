# OpenFranko

Open source C++ implementation of Franko: The Crazy Revenge engine

Motivation:
Franko: The Crazy Revenge is a Polish cult classic video game from the Amiga computer. The goal of this project is to make a implementation that will run on any computer/operating system, and will be modable.

THE PROJECT IS WIP

# Build instructions

## Linux

Install dependencies:

Debian/Ubuntu

```
sudo apt update
sudo apt upgrade
sudo apt install build-essential cmake git libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev catch2
```

Arch

```
sudo pacman -Syu
sudo pacman -S base-devel cmake git sdl2 sdl2_image sdl2_mixer catch2
```

Compilation:

```
git clone https://github.com/OpenFranko/OpenFranko.git
cd OpenFranko
mkdir build && cd build
cmake -DBUILD_TOOLS=ON .. 
cmake --build -j $(nproc)
```

Executables can be located in the build directory
