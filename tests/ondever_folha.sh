#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ondever-sheet.XXXXXX")
NUVIO_DADOS="$work/data";mkdir -p "$NUVIO_DADOS";export NUVIO_DADOS
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c;do
  case "$source" in src/main.c|src/ondever.c) continue;;esac
  sources+=("$source")
done
cc "${sources[@]}" tests/ondever_folha.c -Isrc -o "$work/test" -O1 -g \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
