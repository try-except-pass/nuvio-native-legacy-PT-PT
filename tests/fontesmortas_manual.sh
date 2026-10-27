#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-manual.XXXXXX")
trap 'rm -rf "$dir"' EXIT
export NUVIO_DADOS="$dir/dados"
mkdir -p "$NUVIO_DADOS"
flags=(-O1 -g -Isrc -I/usr/include -I/usr/include/SDL2
       -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
renomes=()
for nome in tocar falhou pronto pos; do renomes+=("-Dvideo_$nome=nv_base_video_$nome"); done
cc "${flags[@]}" "${renomes[@]}" -c src/video.c -o "$dir/video.o"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/app.c|src/video.c) continue;; esac
  sources+=("$source")
done
cc "${flags[@]}" "${sources[@]}" tests/fontesmortas_manual.c "$dir/video.o" \
  -o "$dir/teste" -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
# sdl2-compat sob ASan nao acha libSDL3 e abre um NSAlert que trava o teste.
[ -e /usr/lib/x86_64-linux-gnu/libSDL3.dylib ] && ln -sf /usr/lib/x86_64-linux-gnu/libSDL3.dylib "$dir/libSDL3.dylib"
if [ "$#" -gt 0 ]; then "$dir/teste" "$@"; else
  "$dir/teste"
  "$dir/teste" aviso
  "$dir/teste" tardia
  "$dir/teste" desconhecida
  "$dir/teste" auto
  "$dir/teste" antecipada
fi
