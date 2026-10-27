#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/tmp}"
mkdir -p "$TMPDIR"
work=$(mktemp -d "$TMPDIR/nuvio-avisofonte.XXXXXX")
export NUVIO_DADOS="$work/data"
mkdir -p "$NUVIO_DADOS"
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/agenda.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/avisofonte.c -Isrc -o "$work/test" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
