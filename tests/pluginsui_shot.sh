#!/bin/bash
# Capturas da tela de Plugins (tests/pluginsui_shot.c). Sem rede.
#   PLUG_MANIFESTO=<manifest.json> bash tests/pluginsui_shot.sh [/tmp/prefixo]
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/pluginsui-shot.XXXXXX")
NUVIO_DADOS="$tmp/dados"; mkdir -p "$NUVIO_DADOS"; export NUVIO_DADOS
trap 'rm -rf "$tmp"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do case "$source" in src/main.c) continue;; esac; sources+=("$source"); done
cc "${sources[@]}" tests/pluginsui_shot.c -Isrc -o "$tmp/shot" -O0 -g \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image \
  -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/shot" "$@"
