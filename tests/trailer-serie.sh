#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# #123: trailers de serie no detalhe (ficha TMDB com videos en/null, pt-BR).
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Wextra -DNUVIO_TRAILER_TEST -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -ffunction-sections -fdata-sections \
  src/extras.c src/notasfontes.c src/trailerfonte.c src/js.c src/rede.c src/redeurl.c tests/trailer-serie.c \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lcurl -lpthread -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -o ${TMPDIR:-/tmp}/nuvio-trailer-serie-tests
${TMPDIR:-/tmp}/nuvio-trailer-serie-tests
