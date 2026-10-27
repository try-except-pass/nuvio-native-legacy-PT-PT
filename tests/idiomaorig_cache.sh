#!/bin/bash
# Busca do idioma original com rede de verdade (TMDB + Wikidata). Precisa da chave do TMDB:
#   bash tests/idiomaorig_cache.sh            (le a chave de local.properties)
set -euo pipefail
cd "$(dirname "$0")/.."
K=${TMDB_KEY:-$(grep -i tmdb local.properties | head -1 | cut -d= -f2-)}
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-idiomaorig.XXXXXX")
trap 'rm -rf "$work"' EXIT
srcs=$(ls src/*.c src/dts/*.c | grep -v "^src/main\.c$")
cc $srcs tests/idiomaorig_cache.c -Isrc $(sdl2-config --cflags) -O1 -w -o "$work/t" \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl -lcurl
"$work/t" "$K" "$work/cache.txt" | grep -v '^\[rede\]'
