#!/bin/bash
# Local reproduction with real GL and ASan/UBSan; isolated data, no TV needed.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-descanso-wake.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" = src/main.c ] || sources+=("$source")
done
cc "${sources[@]}" tests/descanso_wake.c -Isrc -o "$work/test" \
  -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
# Same sdl2-compat/ASan loader workaround as fontesmortas_manual.sh.
[ ! -e /usr/lib/x86_64-linux-gnu/libSDL3.dylib ] || ln -s /usr/lib/x86_64-linux-gnu/libSDL3.dylib "$work/libSDL3.dylib"
NUVIO_DADOS="$work" "$work/test"

# #205: a corrida do encerramento com o barramento LS2. O callback do
# screensaver (fio do laco) preso DENTRO do lsChamar enquanto o video_encerrar
# faz o LSUnregister — reproduzida com o ramo luna real de video.c e o laco
# dublado + GLib real com inicio atrasado (tests/descanso_luna.c).
luna_sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/video.c) continue;; esac
  luna_sources+=("$source")
done
glib_flags=($(pkg-config --cflags --libs glib-2.0))
cc "${luna_sources[@]}" tests/descanso_luna.c -Isrc -o "$work/test_luna" "${glib_flags[@]}" \
  -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$work" "$work/test_luna"
