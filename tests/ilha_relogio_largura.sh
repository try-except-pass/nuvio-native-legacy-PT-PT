#!/bin/bash
# A reserva do menu do topo cobre a ilha real do relogio com o medidor ligado. Ver o .c.
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ilha-relogio-largura.XXXXXXXX")
trap 'rm -rf "$dir"' EXIT
cc tests/ilha_relogio_largura.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -Wl,--unresolved-symbols=ignore-all -o "$dir/test" \
  -Wall -Wno-macro-redefined -Wno-deprecated-declarations -Wno-unused-function
"$dir/test"
