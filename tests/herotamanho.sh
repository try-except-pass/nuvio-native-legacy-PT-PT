#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
cc tests/herotamanho.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-herotamanho" \
  -O1 -g -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined
"${TMPDIR:-/tmp}/nuvio-herotamanho"
