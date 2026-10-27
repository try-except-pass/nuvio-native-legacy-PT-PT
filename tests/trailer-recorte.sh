#!/bin/bash
# trailer_recorte: o "cover" da fonte do trailer no destino (trailer.h).
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/trailer-recorte.c -Isrc -o "$tmp/t" -O0 \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined 2>"$tmp/build.log" || { cat "$tmp/build.log" >&2; exit 1; }
"$tmp/t"
