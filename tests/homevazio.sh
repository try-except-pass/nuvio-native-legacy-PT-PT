#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
export TMPDIR=${TMPDIR:-/tmp}
mkdir -p "$TMPDIR"
bin=$(mktemp "$TMPDIR/homevazio.XXXXXX")
trap 'rm -f "$bin"' EXIT
cc src/homevazio.c src/qr.c tests/homevazio.c -Isrc -o "$bin" -O1 \
  -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$bin"
