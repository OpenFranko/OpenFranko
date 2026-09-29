#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build-dos}
DJGPP_TARGET=${DJGPP_TARGET:-i686-pc-msdosdjgpp}
JOBS=${JOBS:-$(nproc)}

LIBXMP_VERSION=4.7.3
LIBXMP_URL=https://github.com/libxmp/libxmp/releases/download/libxmp-$LIBXMP_VERSION/libxmp-$LIBXMP_VERSION.tar.gz
LIBXMP_SHA256=b6a98797e4fb9c9a705f5d53112aa5214561857e929a644928b9e658930d9440
CWSDPMI_URL=https://www.delorie.com/pub/djgpp/current/v2misc/csdpmi7b.zip
CWSDPMI_SHA256=deacda0488e1cdd7c4a9f32fab45662b34c0ed6b2d7d4d13bc07041b62004a8c

DEPS_DIR=$BUILD_DIR/deps
XMP_PREFIX=$BUILD_DIR/libxmp
GAME_DIR=$BUILD_DIR/game

usage() {
  echo "Usage: $0 [--assets <assets_dir>] [-D<cmake_option>...]"
  echo "Builds the DOS version with DJGPP and Allegro 4 into $BUILD_DIR and"
  echo "puts a runnable copy in $GAME_DIR. -D options are passed to CMake."
  echo "Environment: BUILD_DIR, DJGPP_TARGET ($DJGPP_TARGET), JOBS ($JOBS)."
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
  for tool in cmake pkg-config curl tar unzip sha256sum; do
    command -v "$tool" > /dev/null || fail "$tool not found"
  done
  for tool in "$DJGPP_TARGET-cmake" "$DJGPP_TARGET-gcc" "$DJGPP_TARGET-strip"; do
    command -v "$tool" > /dev/null ||
      fail "$tool not found (install djgpp-gcc and djgpp-cmake)"
  done
  mkdir -p "$DEPS_DIR"
  printf '#include <allegro.h>\nint main(void) { return allegro_init(); }\n' \
    > "$DEPS_DIR/allegro.c"
  "$DJGPP_TARGET-gcc" "$DEPS_DIR/allegro.c" -o "$DEPS_DIR/allegro.exe" \
    -lalleg 2> /dev/null ||
    fail "Allegro 4 for $DJGPP_TARGET not found (install djgpp-allegro4)"
}

build_libxmp() {
  if [ -f "$XMP_PREFIX/lib/libxmp.a" ]; then
    return
  fi
  local archive=$DEPS_DIR/libxmp-$LIBXMP_VERSION.tar.gz
  download "$LIBXMP_URL" "$LIBXMP_SHA256" "$archive"
  echo "Building libxmp $LIBXMP_VERSION"
  rm -rf "$DEPS_DIR/libxmp-$LIBXMP_VERSION" "$DEPS_DIR/libxmp-build"
  tar -xzf "$archive" -C "$DEPS_DIR"
  run_logged "$DEPS_DIR/libxmp-build.log" \
    "$DJGPP_TARGET-cmake" -S "$DEPS_DIR/libxmp-$LIBXMP_VERSION" \
    -B "$DEPS_DIR/libxmp-build" -DBUILD_SHARED=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-march=i586 \
    -DCMAKE_INSTALL_PREFIX:PATH="$XMP_PREFIX" \
    -DCMAKE_INSTALL_LIBDIR:PATH="$XMP_PREFIX/lib"
  run_logged "$DEPS_DIR/libxmp-build.log" \
    cmake --build "$DEPS_DIR/libxmp-build" -j "$JOBS"
  run_logged "$DEPS_DIR/libxmp-install.log" \
    cmake --install "$DEPS_DIR/libxmp-build"
}

build_game() {
  echo "Building OpenFranko"
  PKG_CONFIG_PATH=$XMP_PREFIX/lib/pkgconfig "$DJGPP_TARGET-cmake" \
    -S "$SOURCE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_EXE_LINKER_FLAGS="-L$XMP_PREFIX/lib" "${CMAKE_ARGS[@]}"
  cmake --build "$BUILD_DIR" -j "$JOBS"
}

assemble_game() {
  mkdir -p "$GAME_DIR"
  "$DJGPP_TARGET-strip" -o "$GAME_DIR/franko.exe" "$BUILD_DIR/src/franko.exe"
  if [ ! -f "$GAME_DIR/CWSDPMI.EXE" ]; then
    local archive=$DEPS_DIR/csdpmi7b.zip
    download "$CWSDPMI_URL" "$CWSDPMI_SHA256" "$archive"
    unzip -p "$archive" bin/CWSDPMI.EXE > "$GAME_DIR/CWSDPMI.EXE"
  fi
  if [ -n "$ASSETS_DIR" ] &&
    [ "$(realpath "$ASSETS_DIR")" != "$(realpath -m "$GAME_DIR/assets")" ]; then
    rm -rf "$GAME_DIR/assets"
    cp -r "$ASSETS_DIR" "$GAME_DIR/assets"
  fi
  cat > "$GAME_DIR/dosbox-x.conf" << EOF
[cpu]
cputype=pentium_ii
cycles=max

[dos]
ver=7.1

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
    ASSETS_DIR=$2
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
build_libxmp
build_game
assemble_game

echo "Wrote $GAME_DIR/franko.exe"
if [ -d "$GAME_DIR/assets" ]; then
  echo "Run it with: flatpak run com.dosbox_x.DOSBox-X -conf $GAME_DIR/dosbox-x.conf"
else
  echo "Copy the extracted assets to $GAME_DIR/assets (or pass --assets) to play."
fi
