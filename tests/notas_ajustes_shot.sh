#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Captura de Ajustes > Integracoes > "Notas no titulo" (PNG, sem rede). Nao
# entra na suite: precisa de janela GL e de olho humano.
#   mkdir -p /tmp/nv-notas-shots && bash tests/notas_ajustes_shot.sh /tmp/nv-notas-shots/aj
set -eu
cd "$(dirname "$0")/.."
export NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-notasaj-dados.XXXXXX)
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ]; then sources+=("$source"); fi
done
cc -DAJUSTES_TESTE "${sources[@]}" tests/notas_ajustes_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-notasaj-shot \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined -w
${TMPDIR:-/tmp}/nuvio-notasaj-shot "$@"
