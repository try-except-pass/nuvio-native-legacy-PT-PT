#!/bin/bash
# Offline cast portraits: real metadata fixture, real detail parser/catalog.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cast-addon.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc src/catalogo.c src/cwordem.c tests/cast_addon.c src/cotacat.c \
  src/js.c src/metaprov.c src/colecoes.c src/redeurl.c src/catordem.c src/artefontes.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o "$work/test" -O1 -g -Wall -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
