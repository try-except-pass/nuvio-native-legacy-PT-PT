#!/bin/bash
# Compile shared host objects once, then run existing texture regressions.
set -eu
cd "$(dirname "$0")/.."
obj=$(mktemp -d "${TMPDIR:-/tmp}/nv-cache-regressions.XXXXXX")
trap 'rm -rf "$obj"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/tex_cache.c) continue;; esac
  sources+=("$source")
done
export NV_CACHE_TEST_FLAGS='-O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 -Wno-deprecated-declarations -Wno-macro-redefined'
printf '%s\n' "${sources[@]}" | xargs -P 8 -I{} sh -c \
  'cc $NV_CACHE_TEST_FLAGS -c "$1" -o "$2/$(echo "$1" | tr / _).o"' _ {} "$obj"
for name in texquente texpressao texb3 tex_nonblocking texdisco texbruto texfila teximgloop; do
  cc $NV_CACHE_TEST_FLAGS "tests/$name.c" "$obj"/*.o -o "$obj/$name" \
    -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
  "$obj/$name"
done
bash tests/tex_memory.sh
