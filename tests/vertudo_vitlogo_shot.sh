#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Vitrine da colecao (job 205-vitlogo1015): destaque abaixo do cabecalho e seletor recolhido. Nao
# entra na suite (tools/testa-tudo.sh pula *_shot.sh): precisa de janela GL.
#
#   bash tests/vertudo_vitlogo_shot.sh ${TMPDIR:-/tmp}/rev/vitlogo1015/depois
set -eu
cd "$(dirname "$0")/.."

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/vertudo.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/vertudo_vitlogo_shot.c -Isrc -Itests -o "${NUVIO_SHOT_BIN:-${TMPDIR:-/tmp}/nuvio-vitlogo-shot}" \
  -O1 -g \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
saida="${1:-$TMPDIR/nuvio-vitlogo}"
mkdir -p "$saida"
"${NUVIO_SHOT_BIN:-${TMPDIR:-/tmp}/nuvio-vitlogo-shot}" "$saida"
