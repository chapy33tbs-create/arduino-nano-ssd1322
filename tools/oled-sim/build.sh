#!/usr/bin/env bash
# Build the OLED layout simulator as WebAssembly.
#
# Why WASM and not a native .exe: Smart App Control on this PC blocks newly
# created unsigned executables, so the renderer is compiled to wasm32-wasi and
# executed with Node instead. Native build works on machines without SAC:
#   zig cc -O1 -w -I"$U8G2_CLIB" sim.c "$U8G2_CLIB"/*.c -o sim.exe -lm
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
HERE_NATIVE="$(cygpath -m "$HERE" 2>/dev/null || echo "$HERE")"
U8G2_CLIB="${U8G2_CLIB:-C:/Users/chapy/work/arduino-nano-ssd1322/.pio/libdeps/nanoatmega328/U8g2/src/clib}"
ZIG_PY="${ZIG_PY:-C:/Users/chapy/pio-venv/Scripts/python.exe}"

"$ZIG_PY" -m ziglang cc -target wasm32-wasi -O1 -w -I"$U8G2_CLIB" \
  "$HERE_NATIVE/sim.c" "$U8G2_CLIB"/*.c -o "$HERE_NATIVE/sim.wasm" -lm

echo "built $HERE/sim.wasm"
echo "run: node --experimental-wasi-unstable-preview1 run.mjs 25 80 4.5 1 > out.txt"
