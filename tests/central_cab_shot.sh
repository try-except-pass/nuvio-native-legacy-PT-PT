#!/bin/bash
# Cabecalho da Central de controle na ilha do player. Nao entra na suite
# (*_shot.sh): janela GL. Ver tests/central_cab_shot.c.
#   bash tests/central_cab_shot.sh <prefixo>   -> <prefixo>-central.png
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" != src/main.c ] && sources+=("$source"); done
bin="${TMPDIR:-/tmp}/nuvio-central-cab-shot"
cc -DNV_SHOT_HOOKS -DCENTRAL_TESTE "${sources[@]}" tests/central_cab_shot.c -Isrc -o "$bin" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
D=$(mktemp -d); trap 'rm -rf "$D" "$bin"' EXIT
NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" "$bin" "$1"
