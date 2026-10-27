#!/bin/bash
# Abertura Cinema (#205): a regra da tela de abertura premium. Ver o .c.
set -eu
cd "$(dirname "$0")/.."
cc src/abrecine.c tests/abrecine.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-abrecine" \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu -lSDL2 -lm
"${TMPDIR:-/tmp}/nuvio-abrecine"
