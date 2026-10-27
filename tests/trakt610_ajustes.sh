#!/bin/bash
# Offline Settings behavior plus PNG. Foreground only.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p ${TMPDIR:-/tmp}/rev/trakt610
bin=${TMPDIR:-/tmp}/rev/trakt610/settings-test
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
defs=()
if [ "${TRAKT610_BEFORE:-0}" = 1 ]; then defs+=(-DTRAKT610_CAPTURE_ONLY); fi
cc ${defs[@]+"${defs[@]}"} -DAJUSTES_TESTE "${sources[@]}" tests/trakt610_ajustes.c -Isrc -o "$bin" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$bin" "${1:-${TMPDIR:-/tmp}/rev/trakt610/after.png}"
