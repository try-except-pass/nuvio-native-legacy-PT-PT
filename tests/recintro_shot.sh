#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas das tres paginas do cartao de primeira vez do Social, em BMP, sem
# interacao. Nao entra na suite: precisa de janela GL e de olho humano.
#
#   bash tests/recintro_shot.sh /tmp/nuvio-recintro
#
# src/recomenda.c e INCLUIDO pelo teste (como em tests/social_shot.sh) para que
# NV_REC_URL fique definida: sem ela recomenda_ativo() e 0 e o cartao nao abre —
# que e o comportamento certo do pacote sem servico, e nao o que esta foto quer
# provar.
set -eu
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-recintro-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
echo "dados da captura em $NUVIO_DADOS"

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/recomenda.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/recintro_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-recintro-shot \
  -O1 -g \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-recintro-shot "$@"
