#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Title-line order + Letterboxd survives a late load.
set -eu
cd "$(dirname "$0")/.."
cc tests/notaordem.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-notaordem -Wall -Wextra
${TMPDIR:-/tmp}/nuvio-notaordem

export NUVIO_DADOS=$(mktemp -d "${TMPDIR}/nuvio-notas-dados.XXXXXX")
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c) continue;; esac
  sources+=("$source")
done
cc -DAJUSTES_TESTE "${sources[@]}" tests/notas_shot.c -Isrc -o "${TMPDIR}/nuvio-notaordem-ui" \
  -O0 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir -p ${TMPDIR:-/tmp}/rev/notasep
NUVIO_NOTAORDEM_TEST=1 "${TMPDIR}/nuvio-notaordem-ui" "${1:-${TMPDIR:-/tmp}/rev/notasep/after}"
python3 tests/notaepisodio.py
bash tests/cateps.sh
