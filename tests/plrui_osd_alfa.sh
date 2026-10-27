#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
B=$(mktemp -d); trap 'rm -rf "$B"' EXIT
cc tests/plrui_osd_alfa.c src/plrui.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined -Wl,--unresolved-symbols=ignore-all -o "$B/t"
"$B/t"
