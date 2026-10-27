#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Limite de pedidos de episodios do card terminado do Continuar, por titulo.
#   bash tests/cw_pedidos.sh
set -euo pipefail
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/continuar.c) continue;; esac
  sources+=("$source")
done
binary="$(mktemp ${TMPDIR:-/tmp}/nuvio-cw-pedidos.XXXXXX)"
trap 'rm -f "$binary"' EXIT
cc "${sources[@]}" tests/cw_pedidos.c -Isrc -o "$binary" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$binary" | grep -v '^\[cw\]'
