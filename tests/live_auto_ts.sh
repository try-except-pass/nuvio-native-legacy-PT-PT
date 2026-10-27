#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
flags=(-O1 -g -ffunction-sections -fdata-sections -Isrc -I/usr/include -I/usr/include/SDL2)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
cc "${flags[@]}" tests/live_auto_ts.c -pthread -o "$work/probe"
"$work/probe"
bash tests/guia_stream_headers.sh
