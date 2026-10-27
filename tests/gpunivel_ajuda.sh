#!/bin/bash
# #410B: ajuda dos Ajustes e i18n pelo caminho real; sem GL/janela/rede.
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-gpunivel-ajuda.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc "${sources[@]}" tests/gpunivel_ajuda.c -Isrc -o "$work/teste" \
  -DNV_GPUN_TESTE -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$work" "$work/teste"
