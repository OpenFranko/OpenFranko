#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build-jaguar}
JOBS=${JOBS:-$(nproc)}

TOOLCHAIN_URL=https://github.com/haarer/toolchain68k/releases/download/gcc152-update1/toolchain-m68k-elf-linux-gcc-15.2.0.tar.gz
TOOLCHAIN_SHA256=ad41506ab6c694d0566f4d3b97d44ab301376cc01ecb5174789d50ad236500e7
SDK_URL=https://github.com/cubanismo/jaguar-sdk.git
SDK_COMMIT=b806b6fb8c8f18f3a0e7ce1eef841a0a212bf480

DEPS_DIR=$BUILD_DIR/deps
TOOLCHAIN_DIR=$BUILD_DIR/toolchain
SDK_DIR=$BUILD_DIR/jaguar-sdk
PREFIX=${JAGUAR_TOOLCHAIN_PREFIX:-$TOOLCHAIN_DIR/bin/m68k-elf-}
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
  echo "Builds the Atari Jaguar version with an m68k-elf GCC and rmac into"
  echo "$BUILD_DIR. Unless JAGUAR_TOOLCHAIN_PREFIX names an installed"
  echo "toolchain (e.g. m68k-elf-), GCC 15.2 with newlib is downloaded into"
  echo "$TOOLCHAIN_DIR first. Unless --sdk is given or rmac is on the PATH,"
  echo "the Jaguar SDK is fetched into $SDK_DIR and its rmac and"
  echo "jagcrypt are built. With --assets it packs the extracted game data"
  echo "into a cartridge image, $GAME_DIR/franko.j64."
  echo "-D options are passed to CMake. Environment: BUILD_DIR, JOBS ($JOBS),"
  echo "JAGUAR_TOOLCHAIN_PREFIX, JAGSDK."
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

install_toolchain() {
  if [ -n "${JAGUAR_TOOLCHAIN_PREFIX:-}" ] || [ -x "${PREFIX}g++" ]; then
    return
  fi
  local tool
  for tool in curl tar gzip sha256sum; do
    command -v "$tool" > /dev/null || fail "$tool not found"
  done
  mkdir -p "$DEPS_DIR"
  local archive=$DEPS_DIR/${TOOLCHAIN_URL##*/}
  download "$TOOLCHAIN_URL" "$TOOLCHAIN_SHA256" "$archive"
  echo "Unpacking the m68k-elf toolchain into $TOOLCHAIN_DIR"
  rm -rf "$TOOLCHAIN_DIR" "$TOOLCHAIN_DIR.part"
  mkdir -p "$TOOLCHAIN_DIR.part"
  tar -xzf "$archive" -C "$TOOLCHAIN_DIR.part"
  mv "$TOOLCHAIN_DIR.part" "$TOOLCHAIN_DIR"
}

install_sdk() {
  if [ -n "${JAGSDK:-}" ] || command -v rmac > /dev/null; then
    return
  fi
  if [ ! -x "$SDK_DIR/tools/bin/rmac" ] ||
    [ ! -x "$SDK_DIR/tools/bin/jagcrypt" ]; then
    local tool
    for tool in git make; do
      command -v "$tool" > /dev/null || fail "$tool not found"
    done
    echo "Fetching the Jaguar SDK into $SDK_DIR"
    rm -rf "$SDK_DIR"
    git init -q "$SDK_DIR"
    git -C "$SDK_DIR" fetch -q --depth 1 "$SDK_URL" "$SDK_COMMIT" ||
      fail "Failed to fetch $SDK_URL"
    git -C "$SDK_DIR" checkout -q FETCH_HEAD
    git -C "$SDK_DIR" submodule update -q --init tools/src/rmac \
      tools/src/pc_jagcrypt || fail "Failed to fetch rmac and jagcrypt"
    echo "Building rmac and jagcrypt"
    mkdir -p "$SDK_DIR/tools/bin"
    run_logged "$BUILD_DIR/rmac.log" make -C "$SDK_DIR/tools/src/rmac"
    cp "$SDK_DIR/tools/src/rmac/rmac" "$SDK_DIR/tools/bin"
    run_logged "$BUILD_DIR/jagcrypt.log" make -C "$SDK_DIR/tools/src/pc_jagcrypt"
    cp "$SDK_DIR/tools/src/pc_jagcrypt/jagcrypt" "$SDK_DIR/tools/bin"
  fi
  export JAGSDK=$SDK_DIR
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
  local compiler cached rmac
  compiler=$(command -v "${PREFIX}g++")
  if [ -x "${JAGSDK:-}/tools/bin/rmac" ]; then
    rmac=$JAGSDK/tools/bin/rmac
  else
    rmac=$(command -v rmac)
  fi
  if [ -f "$CROSS_DIR/CMakeCache.txt" ]; then
    cached=$(sed -n 's/^CMAKE_CXX_COMPILER:[A-Z]*=//p' "$CROSS_DIR/CMakeCache.txt")
    if [ "$cached" != "$compiler" ]; then
      echo "The cross compiler is now $compiler; reconfiguring $CROSS_DIR"
      rm -rf "$CROSS_DIR"
    fi
  fi
  cmake -S "$SOURCE_DIR" -B "$CROSS_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$SOURCE_DIR/cmake/JaguarToolchain.cmake" \
    -DJAGUAR_TOOLCHAIN_PREFIX="$PREFIX" -DJAGUAR_RMAC="$rmac" \
    -DCMAKE_BUILD_TYPE=Release \
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
install_toolchain
install_sdk
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
