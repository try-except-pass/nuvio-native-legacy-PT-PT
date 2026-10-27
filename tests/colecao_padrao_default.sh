#!/bin/bash
# Regression for the profile default collection view without GL/UI paths.
set -euo pipefail
cd "$(dirname "$0")/.."
sources=()
flags=(-O1 -g)
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/vertudo.c) continue;; esac
  sources+=("$source")
done
bin="$(mktemp "${TMPDIR:-/tmp}/nuvio-colecao-padrao-default.XXXXXXXX")"
trap 'rm -f "$bin"' EXIT
"${CC:-cc}" "${sources[@]}" tests/colecao_padrao_default.c -Isrc -o "$bin" "${flags[@]}" \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
"$bin"
