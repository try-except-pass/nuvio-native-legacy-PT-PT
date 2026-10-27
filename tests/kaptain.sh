#!/bin/bash
# Offline Kaptain URL parity and split-upcoming regression.
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=${TMPDIR:-/tmp}
out=$(mktemp -d "$TMPDIR/kaptain.XXXXXX")
trap 'rm -rf "$out"' EXIT
cc tests/kaptain.c src/colecoes.c src/js.c -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -O1 -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -Wno-deprecated-declarations -o "$out/urls"
"$out/urls"
python3 tests/kaptain_split.py "$out"
