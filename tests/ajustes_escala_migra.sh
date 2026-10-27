#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Migracao unica do tamanho dos Ajustes (2.0.3).
#
#   bash tests/ajustes_escala_migra.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/ajustes_escala_migra.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-ajustes-padroes \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-ajustes-padroes
