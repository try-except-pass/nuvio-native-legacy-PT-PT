#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pr670.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-std=gnu11 -O1 -g -pthread -Isrc -ffunction-sections -fdata-sections
  -I/usr/include -I/usr/include/SDL2 -Wno-deprecated-declarations)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
cc "${flags[@]}" tests/pr670_stream_ids.c src/js.c src/jsw.c src/watchstate.c \
  src/addonstats.c src/fontefalha.c src/idcanal.c -o "$work/test"
if [ "$#" -gt 0 ]; then "$work/test" "$1"; else
  for scenario in merge namespaces specials; do "$work/test" "$scenario"; done
fi
