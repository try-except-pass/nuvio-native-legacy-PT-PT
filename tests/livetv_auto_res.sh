#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
flags=(-ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
cc -Wall -Wextra "${flags[@]}" tests/livetv_auto_res.c -Isrc -I/usr/include -o "$work/test" -lpthread
"$work/test"
