#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas da enquete na ilha (enquete.h): convite, opcoes, resultado, bolinha de
# "Agora nao" e o opt-out com Desfazer, em PNG, contra o servidor de mentira.
# Nao entra na suite.   bash tests/enquete_shot.sh [prefixo]
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-enquete-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/enquete.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/enquete_shot.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-enquete-shot" \
  -O1 -g -DNV_LEVE -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -w
"${TMPDIR:-/tmp}/nuvio-enquete-shot" "${1:-${TMPDIR:-/tmp}/nuvio-enquete}"
