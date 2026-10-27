#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-nextup-art-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
cc src/catalogo.c src/progresso.c src/posterprov.c src/redeurl.c src/colecoes.c src/js.c src/catordem.c src/artehero.c src/cwordem.c src/proximo.c tests/nextup_art.c \
  -Isrc -o "$tmp/test" -O1 -g -ffunction-sections -fdata-sections \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/test"
