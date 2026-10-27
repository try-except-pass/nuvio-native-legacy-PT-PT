#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Redesenho do Social (cliente): nome, feed, perfil do amigo, merge, fila.
#   bash tests/recomenda_social.sh
set -euo pipefail
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-recsoc-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS" ${TMPDIR:-/tmp}/nuvio-recomenda-social' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/recomenda.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/recomenda_social.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-recomenda-social \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined \
  >"$NUVIO_DADOS/build.log" 2>&1 || { cat "$NUVIO_DADOS/build.log" >&2; exit 1; }
grep -E "recomenda(_social)?\.c.*(error|warning)" "$NUVIO_DADOS/build.log" || true
${TMPDIR:-/tmp}/nuvio-recomenda-social
