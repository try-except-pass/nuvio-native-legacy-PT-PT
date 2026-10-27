#!/bin/bash
# Exercise the real guide event handler without network or rendering.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-guia-hold.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/guia.c) continue;; esac
  sources+=("$source")
done
flags=(-std=gnu11 -O0 -g -Isrc)
if [[ "$(uname -s)" == Darwin ]]; then
  flags+=(-I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu)
  libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl)
else
  flags+=(-DNV_LINUX_DESKTOP)
  read -r -a sdk_flags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf)"
  read -r -a sdk_libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf)"
  flags+=("${sdk_flags[@]}")
  libs=("${sdk_libs[@]}" -lz -lGLESv2 -pthread -lm -ldl)
fi
cc "${sources[@]}" tests/guia_hold.c "${flags[@]}" -o "$work/test" \
  "${libs[@]}" -Wno-deprecated-declarations
"$work/test"
