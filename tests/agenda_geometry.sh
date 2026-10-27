#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc tests/agenda_geometry.c src/layout.c -DNV_TOUCH_UI -O1 -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all -o "$work/agenda-geometry"
"$work/agenda-geometry"
