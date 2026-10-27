#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-proxprefetch.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-O1 -g -Isrc -pthread -I/usr/include -I/usr/include/SDL2
       -Wall -Wextra -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/proxprefetch.c src/posplay.c src/proximo.c src/intro.c src/velocidade.c src/addons.c src/fontefalha.c src/addonstats.c src/js.c -o "$dir/teste"
"$dir/teste"
