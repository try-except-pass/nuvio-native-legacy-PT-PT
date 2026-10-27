#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-traktcontracts.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} -DNV_DUBLE_SESSAO src/trakt.c src/traktscrobble.c \
  src/metaep.c src/js.c src/jsw.c tests/stub_fichameta.c tests/trakt_contracts.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o "$dir/teste" -O1 -g -Wall -Wextra -ffunction-sections -fdata-sections \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread
"$dir/teste"
