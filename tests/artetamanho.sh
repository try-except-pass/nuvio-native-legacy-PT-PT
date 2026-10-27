#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# A variante do tamanho do desenho: a regra (artetamanho.c) e o cache
# (texvariante.c: chave, disco e promocao a heroi). Sem rede.
#
#   bash tests/artetamanho.sh
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} src/artetamanho.c tests/artetamanho.c \
  -Isrc -o ${TMPDIR:-/tmp}/nuvio-artetamanho-tests -O1 -g \
  -Wall -Wextra -Wno-deprecated-declarations
${TMPDIR:-/tmp}/nuvio-artetamanho-tests

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
# Cada tamanho do TMDB com a largura real dele (a "original" em 1600).
ffmpeg -nostdin -loglevel error -y -i tests/amostra.jpg -vf scale=780:439 ${TMPDIR:-/tmp}/nuvio-texvariante-780.jpg >/dev/null
ffmpeg -nostdin -loglevel error -y -i tests/amostra.jpg -vf scale=1280:720 ${TMPDIR:-/tmp}/nuvio-texvariante-1280.jpg >/dev/null
ffmpeg -nostdin -loglevel error -y -i tests/amostra.jpg -vf scale=1600:900 ${TMPDIR:-/tmp}/nuvio-texvariante-1600.jpg >/dev/null
cc ${flags[@]+"${flags[@]}"} "${sources[@]}" tests/texvariante.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-texvariante \
  -O1 -g -Wall -Wextra -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-texvariante ${TMPDIR:-/tmp}/nuvio-texvariante-780.jpg \
  ${TMPDIR:-/tmp}/nuvio-texvariante-1280.jpg ${TMPDIR:-/tmp}/nuvio-texvariante-1600.jpg
