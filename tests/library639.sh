#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
bash tests/contaoffline_limites.sh
bash tests/contapend.sh
bash tests/contalib.sh
bash tests/bibfonte.sh
cc tests/trakt610_lista.c src/js.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o "$tmp/trakt" -O1 -g -Wno-deprecated-declarations -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread
"$tmp/trakt"
cc -O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 -Wno-deprecated-declarations \
  src/simkl.c src/js.c src/traktscrobble.c src/metaep.c tests/simkl_cw.c -o "$tmp/simkl" -lpthread
"$tmp/simkl" library639
