#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
for mode in desktop touch; do
  flags=()
  if [ "$mode" = touch ]; then flags+=(-DNV_TOUCH_UI); fi
  cc tests/menu_tvos.c "${flags[@]}" -Isrc -I/usr/include -I/usr/include/SDL2 \
    -Wno-deprecated-declarations -Wno-macro-redefined \
    -Wl,--unresolved-symbols=ignore-all -o ${TMPDIR:-/tmp}/nuvio-menu-tvos
  ${TMPDIR:-/tmp}/nuvio-menu-tvos
done
