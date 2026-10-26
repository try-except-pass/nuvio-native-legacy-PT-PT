#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Home com o catalogo vazio (o crash de arranque da 2.0.20). Linux com GLES2 e
# SDL offscreen; SANITIZE=1 liga AddressSanitizer (o modo que pega o defeito).
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address -fno-omit-frame-pointer); fi
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ]; then sources+=("$source"); fi
done
BIN="$TMPDIR/nuvio-arranque-vazio"
DADOS="$(mktemp -d "$TMPDIR/nuvio-vazio.XXXXXX")"
trap 'rm -rf "$DADOS"' EXIT
cc ${flags[@]+"${flags[@]}"} -O1 -g -w "${sources[@]}" tests/arranque_vazio.c -Isrc \
  $(sdl2-config --cflags) -o "$BIN" -Wl,--allow-multiple-definition \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lGLESv2 -lEGL -lz -lm -lpthread -ldl
NUVIO_DADOS="$DADOS" SDL_VIDEODRIVER=offscreen ASAN_OPTIONS=detect_leaks=0 "$BIN"
