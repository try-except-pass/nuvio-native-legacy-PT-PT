#!/bin/bash
# Real Search/Detail draw paths and episode menu geometry in a hidden GL window.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d /tmp/nv-text-layout-XXXXXX)
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/busca.c|src/detail.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/textoescala_layout_search.c tests/textoescala_layout.c \
  -Isrc -o "$work/test" -O0 \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
