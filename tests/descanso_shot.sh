#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas da tela de descanso (vitrine e relogio). Precisa de janela GL; nao
# entra na suite. Ver tests/descanso_shot.c.
#   bash tests/descanso_shot.sh [prefixo]
set -eu
cd "$(dirname "$0")/.."
fontes=()
for f in src/*.c src/dts/*.c; do [ "$f" != src/main.c ] && fontes+=("$f"); done
cc "${fontes[@]}" tests/descanso_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-descanso-shot \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
D=$(mktemp -d); trap 'rm -rf "$D"' EXIT
NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" NUVIO_SHOT_FONTE="${NUVIO_SHOT_FONTE:-3}" \
  ${TMPDIR:-/tmp}/nuvio-descanso-shot "${1:-${TMPDIR:-/tmp}/nuvio-descanso}"
