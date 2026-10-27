#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Precisa de rede: fala com tv.apple.com.
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Isrc -I/usr/include -I/usr/include/SDL2 src/trailerapple.c src/rede.c src/redeurl.c src/js.c tests/trailerapple.c -L/usr/lib/x86_64-linux-gnu -lSDL2 -lcurl -lpthread -o ${TMPDIR:-/tmp}/nuvio-trailerapple-tests
${TMPDIR:-/tmp}/nuvio-trailerapple-tests
