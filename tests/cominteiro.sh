#!/bin/bash
# Offline host input/render regression and 1080p PNG evidence.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/tmp}"
mkdir -p "$TMPDIR" ${TMPDIR:-/tmp}/rev/cominteiro
work=$(mktemp -d "$TMPDIR/nuvio-cominteiro.XXXXXX")
trap 'rm -rf "$work"' EXIT
export NUVIO_DADOS="$work/data"
mkdir -p "$NUVIO_DADOS"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c|src/sinopse.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/cominteiro.c -Isrc -o "$work/test" -O1 -g \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
cc -O1 -g -DNV_EXTRAS_TESTE -Isrc -I/usr/include -I/usr/include/SDL2 \
  src/extras.c src/notasfontes.c src/js.c tests/cominteiro_data.c -lpthread -o "$work/data-test"
"$work/data-test"
