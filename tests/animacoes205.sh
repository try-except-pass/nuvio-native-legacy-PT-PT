#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# #205: os focos/estados que ganharam mola NAO podem saltar mais — o valor
# INTERMEDIARIO e conferido quadro a quadro (60 fps), no meio do voo.
#   bash tests/animacoes205.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in
    src/main.c|src/ajustes.c|src/central.c|src/episodios.c|src/aovivoui.c|src/plrui.c) continue;;
  esac
  sources+=("$source")
done
binary="$(mktemp ${TMPDIR:-/tmp}/nuvio-animacoes205.XXXXXX)"
trap 'rm -f "$binary"' EXIT
cc "${sources[@]}" tests/animacoes205.c -Isrc -o "$binary" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$binary"
