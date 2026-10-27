#!/bin/bash
# Preparation regressions; real callbacks, no network or playback.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fn-preparo.XXXXXX")
trap 'rm -rf "$dir"' EXIT
export NUVIO_DADOS="$dir"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/streams.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/fontenota_preparo.c -Isrc -o "$dir/teste" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$dir/teste"
