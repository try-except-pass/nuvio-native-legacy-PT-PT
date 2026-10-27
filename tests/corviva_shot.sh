#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Home e pagina de titulo com "Da arte", "Gradiente", "Imersiva" e "Textura", em BMP,
# sem rede (artes de deploy/app/art). Nao entra na suite (testa-tudo.sh pula
# *_shot.sh): precisa de janela GL e de olho humano.
#
#   bash tests/corviva_shot.sh /tmp/nuvio-corviva
#   ffmpeg -nostdin -loglevel error -y -i /tmp/nuvio-corviva-1-home-dinamica-vermelho.bmp x.png
set -euo pipefail
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-corviva-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/corviva_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-corviva-shot \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined 2>&1 \
  | grep -E "corviva_shot|error" || true
${TMPDIR:-/tmp}/nuvio-corviva-shot "$@"
