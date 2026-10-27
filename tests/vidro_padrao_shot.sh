#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Grade antes/depois do vidro padrao (tests/vidro_padrao_shot.c), glass forcado ligado.
#   bash tests/vidro_padrao_shot.sh /tmp/vidro-padrao-grade.bmp
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-vidro-padrao-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ]; then sources+=("$source"); fi
done
cc "${sources[@]}" tests/vidro_padrao_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-vidro-padrao-shot \
  -O1 -g -DNV_VIDRO_TESTE -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-vidro-padrao-shot "$@"
