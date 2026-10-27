#!/bin/bash
# Pedido de amizade na ilha do relogio: "dito" so depois de aparecer. Ver o .c.
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ilha-pedidos.XXXXXXXX")
trap 'rm -rf "$dir"' EXIT
cc tests/ilha_pedidos.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -Wl,--unresolved-symbols=ignore-all -o "$dir/test" \
  -Wall -Wno-macro-redefined -Wno-deprecated-declarations -Wno-unused-function
"$dir/test"
