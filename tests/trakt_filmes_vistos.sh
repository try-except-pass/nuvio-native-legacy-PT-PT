#!/bin/bash
# Sem SDL, credenciais ou rede; fixture inclui respostas maiores que 100.
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-trakt-filmes.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} -DNV_DUBLE_SESSAO tests/trakt_filmes_vistos.c src/js.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o "$dir/teste" -O1 -g -Wall -Wextra -ffunction-sections -fdata-sections \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread
"$dir/teste"
