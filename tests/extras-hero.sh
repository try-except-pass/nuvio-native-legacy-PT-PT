#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Wextra -DNUVIO_TRAILER_TEST -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -ffunction-sections -fdata-sections \
  src/extras.c src/notasfontes.c src/js.c src/rede.c src/redeurl.c tests/extras-hero.c \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lcurl -lpthread -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -o ${TMPDIR:-/tmp}/nuvio-extras-hero-tests
${TMPDIR:-/tmp}/nuvio-extras-hero-tests
