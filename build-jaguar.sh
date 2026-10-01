#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build-jaguar}
JOBS=${JOBS:-$(nproc)}
PREFIX=${JAGUAR_TOOLCHAIN_PREFIX:-m68k-elf-}

HOST_DIR=$BUILD_DIR/host
CROSS_DIR=$BUILD_DIR/m68k
GAME_DIR=$BUILD_DIR/game
CART_BASE=$((0x800000))
CODE_BASE=$((0x802000))
HEADER_OFFSET=$((0x400))
CART_SIZE=$((0x400000))
MAX_CART_SIZE=$((0x600000))

usage() {
  echo "Usage: $0 [--assets <assets_dir>] [--sdk <jaguar_sdk_dir>] [-D<cmake_option>...]"
  echo "Builds the Atari Jaguar version with an ${PREFIX} cross toolchain and"
  echo "rmac into $BUILD_DIR. With --assets it packs the extracted game data"
  echo "into a cartridge image, $GAME_DIR/franko.j64."
  echo "-D options are passed to CMake. Environment: BUILD_DIR, JOBS ($JOBS),"
  echo "JAGUAR_TOOLCHAIN_PREFIX ($PREFIX), JAGSDK."
}

fail() {
  echo "Error: $*" >&2
  exit 1
}

check_tools() {
  local tool
  for tool in cmake "${PREFIX}g++" "${PREFIX}objcopy" "${PREFIX}nm" dd stat; do
    command -v "$tool" > /dev/null || fail "$tool not found"
  done
  if ! command -v rmac > /dev/null && [ ! -x "${JAGSDK:-}/tools/bin/rmac" ]; then
    fail "rmac not found; put it on PATH or pass --sdk <jaguar_sdk_dir>"
  fi
}

build_host_tools() {
  echo "Building packAssets"
  cmake -S "$SOURCE_DIR" -B "$HOST_DIR" -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TOOLS=ON > "$BUILD_DIR/host-configure.log" 2>&1 ||
    fail "Configuring the host tools failed, see $BUILD_DIR/host-configure.log"
  cmake --build "$HOST_DIR" --target packAssets -j "$JOBS"
}

build_game() {
  echo "Building OpenFranko for the Jaguar"
  cmake -S "$SOURCE_DIR" -B "$CROSS_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$SOURCE_DIR/cmake/JaguarToolchain.cmake" \
    -DJAGUAR_TOOLCHAIN_PREFIX="$PREFIX" -DCMAKE_BUILD_TYPE=Release \
    "${CMAKE_ARGS[@]}"
  cmake --build "$CROSS_DIR" --target OpenFranko -j "$JOBS"
}

write_header() {
  local image=$1
  head -c "$HEADER_OFFSET" /dev/zero > "$image"
  printf '\x04\x04\x04\x04\x00\x80\x20\x00\x00\x00\x00\x00' >> "$image"
  local filler=$((CODE_BASE - CART_BASE - HEADER_OFFSET - 12))
  head -c "$filler" /dev/zero | tr '\0' '\377' >> "$image"
}

assemble_cartridge() {
  local assets=$1
  local elf=$CROSS_DIR/src/franko.elf
  local binary=$CROSS_DIR/src/franko.bin
  local archive=$BUILD_DIR/assets.ofpa
  local payload=$BUILD_DIR/cart.bin
  mkdir -p "$GAME_DIR"
  echo "Packing $assets"
  "$HOST_DIR/tools/converter/packAssets" -i "$assets" -o "$archive"
  local rom_end
  rom_end=$("${PREFIX}nm" "$elf" | awk '$3 == "__rom_end" { print $1 }')
  [ -n "$rom_end" ] || fail "__rom_end not found in $elf"
  local code_size=$((0x$rom_end - CODE_BASE))
  cp "$binary" "$payload"
  truncate -s "$code_size" "$payload"
  cat "$archive" >> "$payload"
  local size
  size=$(stat -c %s "$payload")
  local total=$((CODE_BASE - CART_BASE + size))
  [ "$total" -le "$MAX_CART_SIZE" ] ||
    fail "The cartridge needs $total bytes, more than the 6 MB a Jaguar cartridge holds"
  local cart_size=$CART_SIZE
  if [ "$total" -gt "$CART_SIZE" ]; then
    cart_size=$MAX_CART_SIZE
  fi
  local jagcrypt=
  if command -v jagcrypt > /dev/null; then
    jagcrypt=$(command -v jagcrypt)
  elif [ -x "${JAGSDK:-}/tools/bin/jagcrypt" ]; then
    jagcrypt=$JAGSDK/tools/bin/jagcrypt
  fi
  local image=$GAME_DIR/franko.j64
  if [ -n "$jagcrypt" ] && [ "$cart_size" -eq "$CART_SIZE" ]; then
    echo "Signing the cartridge with $jagcrypt"
    local work
    work=$(mktemp -d)
    cp "$payload" "$work/cart.bin"
    (cd "$work" && "$jagcrypt" -u cart.bin > jagcrypt.log 2>&1) ||
      fail "jagcrypt failed, see $work/jagcrypt.log"
    cp "$work/cart.U1" "$image"
    rm -rf "$work"
  else
    write_header "$image"
    cat "$payload" >> "$image"
    local padding=$((cart_size - $(stat -c %s "$image")))
    head -c "$padding" /dev/zero | tr '\0' '\377' >> "$image"
  fi
  rm -f "$payload"
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
  --sdk)
    [ $# -ge 2 ] || fail "--sdk needs a directory"
    [ -d "$2" ] || fail "No such directory: $2"
    export JAGSDK
    JAGSDK=$(realpath "$2")
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

mkdir -p "$BUILD_DIR"
check_tools
build_game
if [ -n "$ASSETS_DIR" ]; then
  build_host_tools
  assemble_cartridge "$ASSETS_DIR"
  echo "Wrote $GAME_DIR/franko.j64"
else
  echo "Built $CROSS_DIR/src/franko.elf"
  echo "Pass --assets <assets_dir> to pack the extracted assets into a cartridge."
fi
