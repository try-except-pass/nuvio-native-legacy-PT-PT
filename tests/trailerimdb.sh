#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Precisa de rede: fala com api.graphql.imdb.com.
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Isrc -I/usr/include -I/usr/include/SDL2 src/trailerimdb.c src/perfiltv.c src/rede.c src/redeurl.c src/js.c tests/trailerimdb.c -L/usr/lib/x86_64-linux-gnu -lSDL2 -lcurl -lpthread -o ${TMPDIR:-/tmp}/nuvio-trailerimdb-tests
${TMPDIR:-/tmp}/nuvio-trailerimdb-tests "${1:-tt2012616}"
