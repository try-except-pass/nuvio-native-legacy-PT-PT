#!/bin/bash
# Ordenacao por visto da Biblioteca (#352). Roda sem janela.
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ordvisto-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ] && [ "$source" != src/biblioteca.c ]; then sources+=("$source"); fi
done
cc "${sources[@]}" tests/biblioteca_ordem_visto.c -Isrc -o "$tmp/t" \
  -DNV_TRAKT_CLIENT_ID='"chave-de-teste"' -O1 -g \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir -p "$tmp/dados"
NUVIO_DADOS="$tmp/dados" "$tmp/t"
