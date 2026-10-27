#!/bin/bash
# #366 Ocultar filmes assistidos: padrao desligado, por perfil, so filmes vistos.
#
#   bash tests/ocultar_filmes_vistos.sh
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ocultar-vistos-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
export NUVIO_DADOS="$tmp/dados"; mkdir -p "$NUVIO_DADOS"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/ocultar_filmes_vistos.c -Isrc -o "$tmp/teste" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/teste"
