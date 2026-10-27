#!/bin/bash
# Friend profile states (tests/amigoperfil_estados.c), no GL. NUVIO_DADOS is temporary.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-amigoperfil-estados-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/recomenda.c|src/amigoperfil.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/amigoperfil_estados.c -Isrc -o "$tmp/teste" -O1 -g \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -w
mkdir -p "$tmp/dados"
NUVIO_DADOS="$tmp/dados" "$tmp/teste"
