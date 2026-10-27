#!/bin/bash
# Focused PR #526 regressions; no network or graphics context.
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c|src/episodios.c) continue;; esac
  sources+=("$source")
done
binary=$(mktemp /tmp/nuvio-synopsis-review.XXXXXX)
trap 'rm -f "$binary"' EXIT
cc "${sources[@]}" tests/synopsis_review.c -Isrc -o "$binary" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
if [ "$#" -gt 0 ]; then "$binary" "$1"; else
  "$binary" setting
  "$binary" sheet
fi
