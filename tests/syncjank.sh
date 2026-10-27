#!/bin/bash
# Real settings/sync paths; no network, credentials, graphics, or TV required.
set -euo pipefail
cd "$(dirname "$0")/.."
bin="$(mktemp "${TMPDIR:-/tmp}/nuvio-syncjank.XXXXXXXX")"
dir="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-syncjank-dir.XXXXXXXX")"
trap 'rm -rf "$bin" "$dir"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c|src/nuvem.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/syncjank.c -Isrc -o "$bin" -O1 -g -pthread \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$bin" "$dir"
