#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
python3 tests/tex_memory.py
out=$(mktemp "${TMPDIR:-/tmp}/nv-tex-memory.XXXXXX")
trap 'rm -f "$out"' EXIT
cc tests/tex_memory.c src/perfiltv.c -Isrc \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -ljpeg -lpthread -o "$out"
"$out"
