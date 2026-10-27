#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-pausa-lg.XXXXXX")
trap 'rm -rf "$dir"' EXIT
export NUVIO_DADOS="$dir/dados"
mkdir -p "$NUVIO_DADOS"
flags=(-O1 -g -Isrc -I/usr/include -I/usr/include/SDL2
  -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/video.c) continue;; esac
  sources+=("$source")
done
cc -DAJUSTES_TESTE "${flags[@]}" "${sources[@]}" tests/video_pausa_lg.c -o "$dir/test" \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
# Mesmo ajuste do loader sdl2-compat/ASan usado em descanso_wake.sh.
[ ! -e /usr/lib/x86_64-linux-gnu/libSDL3.dylib ] || ln -s /usr/lib/x86_64-linux-gnu/libSDL3.dylib "$dir/libSDL3.dylib"
"$dir/test"
