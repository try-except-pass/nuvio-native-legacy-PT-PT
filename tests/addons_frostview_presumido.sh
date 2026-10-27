#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-frostview-presumido.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -std=gnu11 -O1 -g -pthread -Isrc -ffunction-sections -fdata-sections \
  -I/usr/include -I/usr/include/SDL2 -ffunction-sections -fdata-sections -Wl,--gc-sections \
  tests/addons_frostview_presumido.c src/js.c src/jsw.c src/watchstate.c \
  src/addonstats.c src/fontefalha.c src/idcanal.c src/linguas.c src/legextras.c -o "$work/test"
"$work/test"
