#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-posplay-recuo.XXXXXX")
trap 'rm -rf "$dir"' EXIT
mkdir "$dir/dados"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/posplay.c) continue;; esac
  sources+=("$source")
done
cc -DNV_SHOT_HOOKS "${sources[@]}" tests/posplay_recuo.c -Isrc -o "$dir/test" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$dir/dados" "$dir/test"
