#!/bin/bash
# Barra no topo: a tira de abas do PlxNative. Ver o .c.
#
#   bash tests/menu_topo_tira.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
cc -DNV_TOUCH_UI -DNV_TESTE "${sources[@]}" tests/menu_topo_tira.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-menu-topo-tira" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"${TMPDIR:-/tmp}/nuvio-menu-topo-tira"
