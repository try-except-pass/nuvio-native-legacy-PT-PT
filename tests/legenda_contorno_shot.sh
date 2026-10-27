#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Subtitle outline shot at 1080p (GL window, not in
# the suite: *_shot.sh). See tests/legenda_contorno_shot.c.
#   bash tests/legenda_contorno_shot.sh <dir> [antigo]
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do [ "$source" != src/main.c ] && sources+=("$source"); done
bin="${TMPDIR:-/tmp}/nuvio-legenda-contorno-shot"
cc "${sources[@]}" tests/legenda_contorno_shot.c -Isrc -o "$bin" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
D=$(mktemp -d); trap 'rm -rf "$D" "$bin"' EXIT
mkdir -p "${1:-${TMPDIR:-/tmp}/nv-legenda-contorno}"
NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" "$bin" "${1:-${TMPDIR:-/tmp}/nv-legenda-contorno}" "${2:-}"
