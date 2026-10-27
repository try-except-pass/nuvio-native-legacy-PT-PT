#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ordem400.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
# O teste inclui src/streams.c para observar a ordem realmente montada.
cc "${flags[@]}" tests/fonte_ordem_addon.c src/fontesmortas.c src/stream_parse.c src/fontefiltro.c src/linguas.c src/js.c src/badges.c src/streamfit.c src/naovideo.c -pthread -o "$dir/test"
"$dir/test"
