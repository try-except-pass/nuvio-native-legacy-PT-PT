#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-busca-titulo.XXXXXX")
trap 'rm -rf "$work"' EXIT
export NUVIO_DADOS="$work"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/busca.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/busca_titulo.c -Isrc -o "$work/test" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
