#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=${TMPDIR:-/tmp}
mkdir -p "$TMPDIR"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
out="$(mktemp "$TMPDIR/nuvio-plx-arte.XXXXXX")"
trap 'rm -f "$out"' EXIT
cc ${PLX_TEST_CFLAGS:-} "${sources[@]}" tests/plx_arte.c -Isrc -o "$out" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$out"
