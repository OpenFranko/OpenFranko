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
sudo apt install
```

Arch

```
sudo pacman -S
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
