#!/bin/bash
# #356: episodio "a seguir" ausente na ficha do Nuvio mas presente no Cinemeta.
#   bash tests/trakt_ep_cinemeta.sh
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-trakt-ep.XXXXXX")
trap 'rm -rf "$dir"' EXIT
cc src/trakt.c src/metaep.c tests/stub_fichameta.c src/js.c src/metaprov.c tests/trakt_ep_cinemeta.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o "$dir/teste" -O1 -g -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread
"$dir/teste" | tail -3
