#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build-dos}
JOBS=${JOBS:-$(nproc)}

TARGET=i586-pc-msdosdjgpp
TOOLCHAIN_URL=https://github.com/andrewwutw/build-djgpp/releases/download/v3.4/djgpp-linux64-gcc1220.tar.bz2
TOOLCHAIN_SHA256=8464f17017d6ab1b2bb2df4ed82357b5bf692e6e2b7fee37e315638f3d505f00
ALLEGRO_URL=https://www.delorie.com/pub/djgpp/current/v2tk/allegro/all422ar2.zip
ALLEGRO_SHA256=bbb1c63f3475607c78cc6ef4c9494e1e2f0bd2b8a6825d6ea11d3b5888ac38eb
LIBXMP_VERSION=4.7.3
LIBXMP_URL=https://github.com/libxmp/libxmp/releases/download/libxmp-$LIBXMP_VERSION/libxmp-$LIBXMP_VERSION.tar.gz
LIBXMP_SHA256=b6a98797e4fb9c9a705f5d53112aa5214561857e929a644928b9e658930d9440
CWSDPMI_URL=https://www.delorie.com/pub/djgpp/current/v2misc/csdpmi7b.zip
CWSDPMI_SHA256=deacda0488e1cdd7c4a9f32fab45662b34c0ed6b2d7d4d13bc07041b62004a8c

DEPS_DIR=$BUILD_DIR/deps
DJGPP_DIR=$BUILD_DIR/djgpp
SYSROOT=$DJGPP_DIR/$TARGET
TOOLCHAIN_FILE=$BUILD_DIR/djgpp.cmake
GAME_DIR=$BUILD_DIR/game

usage() {
  echo "Usage: $0 [--assets <assets_dir>] [-D<cmake_option>...]"
  echo "Builds the DOS version with a local DJGPP ($TARGET) and Allegro 4 into"
  echo "$BUILD_DIR and puts a runnable copy in $GAME_DIR."
  echo "-D options are passed to CMake. Environment: BUILD_DIR, JOBS ($JOBS)."
}

fail() {
  echo "Error: $*" >&2
  exit 1
}

run_logged() {
  local log=$1
  shift
  if ! "$@" > "$log" 2>&1; then
    tail -n 20 "$log" >&2
    fail "$1 failed, see $log"
  fi
}

download() {
  local url=$1 sha256=$2 file=$3
  if [ ! -f "$file" ]; then
    echo "Downloading $url"
    curl -fL --retry 3 -o "$file.part" "$url" || fail "Failed to download $url"
    mv "$file.part" "$file"
  fi
  echo "$sha256  $file" | sha256sum -c --quiet - ||
    fail "Checksum mismatch for $file"
}

check_tools() {
  local tool
  for tool in cmake make pkg-config curl tar bzip2 unzip sha256sum; do
    command -v "$tool" > /dev/null || fail "$tool not found"
  done
  mkdir -p "$DEPS_DIR"
}

install_toolchain() {
  if [ ! -x "$DJGPP_DIR/bin/$TARGET-g++" ]; then
    local archive=$DEPS_DIR/djgpp-linux64-gcc1220.tar.bz2
    download "$TOOLCHAIN_URL" "$TOOLCHAIN_SHA256" "$archive"
    echo "Unpacking DJGPP"
    rm -rf "$DJGPP_DIR"
    tar -xjf "$archive" -C "$BUILD_DIR"
  fi
  cat > "$TOOLCHAIN_FILE" << EOF
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i586)
set(CMAKE_C_COMPILER "$DJGPP_DIR/bin/$TARGET-gcc")
set(CMAKE_CXX_COMPILER "$DJGPP_DIR/bin/$TARGET-g++")
set(CMAKE_FIND_ROOT_PATH "$SYSROOT")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(ENV{PKG_CONFIG_LIBDIR} "$SYSROOT/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
EOF
}

install_allegro() {
  if [ -f "$SYSROOT/lib/liballeg.a" ]; then
    return
  fi
  local archive=$DEPS_DIR/all422ar2.zip
  download "$ALLEGRO_URL" "$ALLEGRO_SHA256" "$archive"
  echo "Installing Allegro 4.2.2"
  unzip -q -o "$archive" 'include/*' lib/liballeg.a -d "$SYSROOT"
}

build_libxmp() {
  if [ -f "$SYSROOT/lib/libxmp.a" ]; then
    return
  fi
  local archive=$DEPS_DIR/libxmp-$LIBXMP_VERSION.tar.gz
  download "$LIBXMP_URL" "$LIBXMP_SHA256" "$archive"
  echo "Building libxmp $LIBXMP_VERSION"
  rm -rf "$DEPS_DIR/libxmp-$LIBXMP_VERSION" "$DEPS_DIR/libxmp-build"
  tar -xzf "$archive" -C "$DEPS_DIR"
  run_logged "$DEPS_DIR/libxmp-build.log" \
    cmake -S "$DEPS_DIR/libxmp-$LIBXMP_VERSION" -B "$DEPS_DIR/libxmp-build" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -DBUILD_SHARED=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$SYSROOT"
  run_logged "$DEPS_DIR/libxmp-build.log" \
    cmake --build "$DEPS_DIR/libxmp-build" -j "$JOBS"
  run_logged "$DEPS_DIR/libxmp-install.log" \
    cmake --install "$DEPS_DIR/libxmp-build"
}

build_game() {
  echo "Building OpenFranko"
  if [ -f "$BUILD_DIR/CMakeCache.txt" ] &&
    ! grep -qxF "CMAKE_TOOLCHAIN_FILE:FILEPATH=$TOOLCHAIN_FILE" \
      "$BUILD_DIR/CMakeCache.txt"; then
    rm -rf "$BUILD_DIR/CMakeCache.txt" "$BUILD_DIR/CMakeFiles"
  fi
  cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -DCMAKE_BUILD_TYPE=Release \
    "${CMAKE_ARGS[@]}"
  cmake --build "$BUILD_DIR" -j "$JOBS"
}

pack_assets() {
  local assets=$1
  echo "Packing $assets into $GAME_DIR/assets.tar"
  tar --format=ustar --sort=name --owner=0 --group=0 --numeric-owner \
    --transform 's,^\.,assets,' -cf "$GAME_DIR/assets.tar.part" -C "$assets" .
  mv "$GAME_DIR/assets.tar.part" "$GAME_DIR/assets.tar"
}

assemble_game() {
  mkdir -p "$GAME_DIR"
  "$DJGPP_DIR/bin/$TARGET-strip" -o "$GAME_DIR/franko.exe" \
    "$BUILD_DIR/src/franko.exe"
  if [ ! -f "$GAME_DIR/CWSDPMI.EXE" ]; then
    local archive=$DEPS_DIR/csdpmi7b.zip
    download "$CWSDPMI_URL" "$CWSDPMI_SHA256" "$archive"
    unzip -p "$archive" bin/CWSDPMI.EXE > "$GAME_DIR/CWSDPMI.EXE"
  fi
  if [ -n "$ASSETS_DIR" ]; then
    pack_assets "$ASSETS_DIR"
  elif [ ! -f "$GAME_DIR/assets.tar" ] && [ -d "$GAME_DIR/assets" ]; then
    pack_assets "$GAME_DIR/assets"
  fi
  if [ -f "$GAME_DIR/assets.tar" ]; then
    rm -rf "$GAME_DIR/assets"
  fi
  cat > "$GAME_DIR/dosbox.conf" << EOF
[cpu]
cputype=486dx2
cycles=66000

[memory]
memsize=8

[autoexec]
mount c "$GAME_DIR"
c:
franko
EOF
}

ASSETS_DIR=
CMAKE_ARGS=()
while [ $# -gt 0 ]; do
  case $1 in
  --assets)
    [ $# -ge 2 ] || fail "--assets needs a directory"
    [ -d "$2" ] || fail "No such directory: $2"
    ASSETS_DIR=$(realpath "$2")
    shift 2
    ;;
  -D*)
    CMAKE_ARGS+=("$1")
    shift
    ;;
  -h | --help)
    usage
    exit 0
    ;;
  *)
    usage >&2
    exit 1
    ;;
  esac
done

check_tools
install_toolchain
install_allegro
build_libxmp
build_game
assemble_game

echo "Wrote $GAME_DIR/franko.exe"
if [ -f "$GAME_DIR/assets.tar" ]; then
  echo "Run it with: dosbox -conf $GAME_DIR/dosbox.conf"
  echo "or: flatpak run com.dosbox_x.DOSBox-X -conf $GAME_DIR/dosbox.conf"
else
  echo "Pass --assets <assets_dir> to pack the extracted assets and play."
fi
