#!/bin/bash
# Cache VOD de metadados, com addons.c real e HTTP/video/debrid dublados.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fontecache-vod.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -pthread -I/usr/include -I/usr/include/SDL2
       -Wall -Wextra -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/fontecache_vod.c src/addons.c src/idcanal.c src/fontefalha.c src/addonstats.c src/js.c -o "$dir/teste"
"$dir/teste"
