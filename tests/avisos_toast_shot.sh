#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Captura isolada do toast de avisos, com tema Ocean e sem servico de rede.
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-avisos-toast-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ] && [ "$source" != src/avisos.c ]; then
    sources+=("$source")
  fi
done
cc "${sources[@]}" tests/avisos_toast_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-avisos-toast-shot \
  -O1 -g -DNV_LEVE -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-avisos-toast-shot "${1:-${TMPDIR:-/tmp}/nuvio-avisos-toast.bmp}"
