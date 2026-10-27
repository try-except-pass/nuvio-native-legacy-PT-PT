#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-selos-viva.XXXXXX")
trap 'rm -rf "$dir"' EXIT
cc -O1 -g -Isrc -pthread -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -I/usr/include -I/usr/include/SDL2 \
  tests/selos_lista_viva.c src/streams.c src/fontefiltro.c src/linguas.c -o "$dir/teste"
"$dir/teste"
