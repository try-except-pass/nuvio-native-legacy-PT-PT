#!/bin/bash
# Full source-sheet frame regression, local fixtures and hidden GL only.
set -euo pipefail
cd "$(dirname "$0")/.."
FRAME_DIR=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fontes-quadro.XXXXXX")
trap 'rm -rf "$FRAME_DIR"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [[ "$source" != src/main.c && "$source" != src/streams.c ]] && sources+=("$source")
done
cc "${sources[@]}" tests/fontes_quadro.c -Isrc -o "$FRAME_DIR/test" \
  -O1 -g -finstrument-functions -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$FRAME_DIR" "$FRAME_DIR/test" "$@"
