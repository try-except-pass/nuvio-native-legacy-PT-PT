#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" = src/main.c ] || sources+=("$source"); done
cc "${sources[@]}" tests/conexlim_shot.c -Isrc -DAJUSTES_TESTE \
  -o "$work/shot" -O1 -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir -p ${TMPDIR:-/tmp}/rev/conexlim
NUVIO_DADOS="$work" NUVIO_TESTE_DIR="$work" "$work/shot" ${TMPDIR:-/tmp}/rev/conexlim
