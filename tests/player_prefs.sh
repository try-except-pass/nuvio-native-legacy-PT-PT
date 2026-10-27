#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# ISSUE #42: "Settings are not getting saved (in player caption settings
# mainly)". Roda o mesmo binario em DOIS PROCESSOS — "mata o processo e
# reabre" so prova alguma coisa quando os `static` de player.c voltam ao
# valor de fabrica entre uma chamada e outra, o que um so processo no mesmo
# `main` nao demonstra.
set -eu
cd "$(dirname "$0")/.."

DADOS="$(mktemp -d)"
ARTE="$(mktemp -d)"
trap 'rm -rf "$DADOS" "$ARTE"' EXIT

sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ]; then sources+=("$source"); fi
done
cc "${sources[@]}" tests/player_prefs.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-player-prefs-tests \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined

export NUVIO_DADOS="$DADOS"
export NUVIO_ARTE_TESTE="$ARTE"
${TMPDIR:-/tmp}/nuvio-player-prefs-tests escrever
${TMPDIR:-/tmp}/nuvio-player-prefs-tests ler
${TMPDIR:-/tmp}/nuvio-player-prefs-tests migrar
${TMPDIR:-/tmp}/nuvio-player-prefs-tests migrado
