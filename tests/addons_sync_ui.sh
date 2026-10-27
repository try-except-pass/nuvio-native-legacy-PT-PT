#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
bin=$(mktemp "${TMPDIR:-/tmp}/nuvio-addons-sync-ui.XXXXXXXX")
trap 'rm -f "$bin"' EXIT
flags=(-O1 -g -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  src/addonsui.c tests/addons_sync_ui.c -lm -o "$bin"
"$bin"
