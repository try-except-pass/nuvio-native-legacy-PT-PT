#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas da tela "Entre amigos" (social.c) em BMP, sem rede e sem interacao.
# Nao entra na suite: precisa de janela GL e de olho humano para julgar.
#
#   bash tests/social_tela_shot.sh /tmp/nuvio-social-tela
#   NUVIO_RAIL=fixa bash tests/social_tela_shot.sh /tmp/nuvio-social-fixa
set -eu
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-social-tela.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/social.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/social_tela_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-social-tela-shot \
  -O1 -g \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-social-tela-shot "$@"
