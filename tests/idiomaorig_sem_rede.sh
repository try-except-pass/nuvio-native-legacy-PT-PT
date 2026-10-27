#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-idiomaorig-sem-rede.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/idiomaorig_sem_rede.c src/idiomaorig.c src/js.c -Isrc $(sdl2-config --cflags) -O1 -g -Wall \
  -fsanitize=address,undefined -lSDL2 -lpthread -lm -o "$work/t"
ASAN_OPTIONS=detect_leaks=0 "$work/t" | grep -v '^\[idioma-orig\] tt'
