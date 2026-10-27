#!/bin/bash
# Spotlight + teclado da TV (entrada_texto.h) pelo caminho SDL, sem TV. Ver o .c.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-spot-tv-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
NUVIO_DADOS="$tmp/dados"; mkdir -p "$NUVIO_DADOS"; export NUVIO_DADOS
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" = "src/main.c" ] && continue
  sources+=("$source")
done
cc "${sources[@]}" tests/spotlight_tv_texto.c -Isrc -o "$tmp/t" -O1 -g -DNV_TEXTO_SDL_TESTE \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/t" | grep -E '^(PASS|Assert)|\[texto\]' | tail -20
