#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
# Compila em paralelo: em serie passava dos 50 s.
obj=$(mktemp -d "$TMPDIR/nuvio-texpressao.XXXXXX")
trap 'rm -rf "$obj"' EXIT
export NV_FLAGS="-O1 -g -Wall -Wextra -Isrc -I/usr/include -I/usr/include/SDL2 -Wno-deprecated-declarations -Wno-macro-redefined"
printf '%s\n' "${sources[@]}" tests/texpressao.c |
  xargs -P "$(sysctl -n hw.ncpu 2>/dev/null || nproc)" -I{} sh -c 'cc $NV_FLAGS -c "$1" -o "$2/$(echo "$1" | tr / _).o"' _ {} "$obj"
cc "$obj"/*.o -o "$obj/nuvio-texpressao" -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
"$obj/nuvio-texpressao" "$@"
