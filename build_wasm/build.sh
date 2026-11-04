#!/usr/bin/env bash
# Build the SimulIDE web version (Qt for WebAssembly).
#
# Needs: Qt 6.8.3 for linux_gcc_64 (host) and wasm_singlethread (+qtmultimedia),
#        Emscripten 3.1.56 active in the shell (source emsdk_env.sh).
#
# Usage: build_wasm/build.sh <qt-version-dir> [build-dir] [output-dir]
#   e.g. build_wasm/build.sh ~/qt/6.8.3
set -euo pipefail

QT_DIR="$(cd "$1" && pwd)"
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$(mkdir -p "${2:-$SRC_DIR/build_wasm/out/build}" && cd "${2:-$SRC_DIR/build_wasm/out/build}" && pwd)"
DIST_DIR="$(mkdir -p "${3:-$SRC_DIR/build_wasm/out/dist}" && cd "${3:-$SRC_DIR/build_wasm/out/dist}" && pwd)"
JOBS="$(nproc 2>/dev/null || echo 2)"

command -v em++ >/dev/null || { echo "Emscripten not found: source emsdk_env.sh first"; exit 1; }

# zlib for WebAssembly (SimulIDE links -lz) -------------------------------
ZLIB_WASM="$BUILD_DIR/zlib"
if [ ! -f "$ZLIB_WASM/libz.a" ]; then
    rm -rf "$BUILD_DIR/zlib-src"
    git clone -q --depth 1 --branch v1.3.1 https://github.com/madler/zlib.git "$BUILD_DIR/zlib-src"
    mkdir -p "$ZLIB_WASM"
    ( cd "$BUILD_DIR/zlib-src"
      emcc -O2 -DHAVE_UNISTD_H -c adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c \
           gzread.c gzwrite.c inflate.c infback.c inftrees.c inffast.c trees.c uncompr.c zutil.c
      emar rcs "$ZLIB_WASM/libz.a" *.o
      cp zlib.h zconf.h "$ZLIB_WASM/" )
fi
export ZLIB_WASM

# SimulIDE ----------------------------------------------------------------
export PATH="$QT_DIR/gcc_64/bin:$PATH"   # lrelease, moc, rcc, uic
cd "$BUILD_DIR"
"$QT_DIR/gcc_64/bin/qmake6" -qtconf "$QT_DIR/wasm_singlethread/bin/target_qt.conf" \
    "$SRC_DIR/build_wasm/SimulIDE_wasm.pro"
make -j"$JOBS"

# Web site ----------------------------------------------------------------
OUT="$(ls -d "$SRC_DIR"/build_wasm/executables/SimulIDE_*/ | head -1)"
cp "$OUT"/simulide.js "$OUT"/simulide.wasm "$OUT"/qtloader.js "$DIST_DIR/"
cp "$SRC_DIR"/build_wasm/web/* "$DIST_DIR/"
# Component sets from SimulIDE 1.1.0 that the 2.0 installer doesn't offer
mkdir -p "$DIST_DIR/builtin"
rm -f "$DIST_DIR/builtin/data.zip"
( cd "$SRC_DIR/build_wasm/builtin_data" && zip -qr -9 "$DIST_DIR/builtin/data.zip" . )
# Bump the offline cache name so phones pick up new builds
sed -i "s/simulide-v1/simulide-$(date +%Y%m%d%H%M)/" "$DIST_DIR/sw.js"
echo "Web build ready in $DIST_DIR"
