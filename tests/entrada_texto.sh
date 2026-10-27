#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${TMPDIR:-/tmp}/nuvio-entrada-texto"
cc -O1 -Wall -Wextra -DNV_TEXTO_SDL_TESTE -I/usr/include -I/usr/include/SDL2 \
  tests/entrada_texto.c src/entrada_texto.c -L/usr/lib/x86_64-linux-gnu -lSDL2 -o "$OUT"
"$OUT" | grep -v '^\[texto\]'
