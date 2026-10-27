#!/bin/bash
# Icone essencial no nucleo TPK atualizado, sem janela GL nem TV.
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-icone-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" -O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 \
  -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -Wno-deprecated-declarations \
  -include tests/gfx_icone_embutido_gl.h tests/gfx_icone_embutido.c src/gfx.c \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lGLESv2 -lEGL -lm -lpthread -ldl -o "$tmp/teste"
"$tmp/teste"
