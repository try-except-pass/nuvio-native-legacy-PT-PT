#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Fila da ilha do relogio (prioridade, +N, central). Sem janela; ver o .c.
set -eu
cd "$(dirname "$0")/.."
cc tests/ilhafila.c src/ilha.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -Wl,--unresolved-symbols=ignore-all -o ${TMPDIR:-/tmp}/nuvio-ilhafila \
  -Wno-macro-redefined -Wno-deprecated-declarations
${TMPDIR:-/tmp}/nuvio-ilhafila
