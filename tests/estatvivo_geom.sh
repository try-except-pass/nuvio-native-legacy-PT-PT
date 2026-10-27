#!/bin/bash
# Live statistics: stable geometry, clock alignment, internal hints and pinned
# focus/side navigation. Uses an offscreen GL target. See estatvivo_geom.c.
#   bash tests/estatvivo_geom.sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-estatvivo-geom.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" != src/main.c ] && sources+=("$source"); done
flags=()
cc "${flags[@]}" "${sources[@]}" tests/estatvivo_geom.c -Isrc -o "$work/test" -DESTATVIVO_TESTE -DCENTRAL_TESTE \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir -p "$work/dados"
NUVIO_DADOS="$work/dados" "$work/test"
