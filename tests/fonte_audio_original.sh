#!/bin/bash
# Audio original antes de qualidade: ver tests/fonte_audio_original.c. Compila o app inteiro (sem main.c).
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-audio-orig.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [[ "$source" != src/main.c && "$source" != src/streams.c ]] && sources+=("$source")
done
cc "${sources[@]}" tests/fonte_audio_original.c -Isrc $(sdl2-config --cflags) -o "$work/test" -O1 -w \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl -lcurl -lssl -lcrypto
NUVIO_DADOS="$work" "$work/test"
