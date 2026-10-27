#!/bin/bash
# Escopo (TV, perfil+conta, perfil local), migracao 2.1 e copia entre perfis.
#
#   bash tests/ajustes_escopo.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
cc -DAJUSTES_TESTE "${sources[@]}" tests/ajustes_escopo.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-ajustes-escopo" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"${TMPDIR:-/tmp}/nuvio-ajustes-escopo"
