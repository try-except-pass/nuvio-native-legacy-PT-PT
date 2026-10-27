#!/bin/bash
# Focused collection geometry regression; no window or network.
set -euo pipefail
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/vertudo.c) continue;; esac
  sources+=("$source")
done
bin=$(mktemp "${TMPDIR:-/tmp}/nuvio-colecao-layout.XXXXXXXX")
log=$(mktemp "${TMPDIR:-/tmp}/nuvio-colecao-layout-log.XXXXXXXX")
trap 'rm -f "$bin" "$log"' EXIT
if ! "${CC:-cc}" "${sources[@]}" tests/colecao_layout.c -Isrc -o "$bin" -O1 \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined >"$log" 2>&1; then
  cat "$log" >&2
  exit 1
fi
"$bin"
