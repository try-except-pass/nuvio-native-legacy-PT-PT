#!/bin/bash
# Real sync/settings/session code; only account HTTP is replaced.
set -eu
cd "$(dirname "$0")/.."
bin="$(mktemp "${TMPDIR:-/tmp}/nuvio-ajustes-sync.XXXXXXXX")"
dir="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ajustes-sync-dir.XXXXXXXX")"
trap 'rm -rf "$bin" "$dir"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c|src/nuvem.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/ajustes_sync.c -Isrc -o "$bin" -O0 -g -pthread \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
rc=0
for scenario in ${*:-roundtrip defaults empty offline legacy webedit quota}; do
  mkdir "$dir/$scenario"
  "$bin" "$dir/$scenario" "$scenario" || rc=1
done
exit "$rc"
