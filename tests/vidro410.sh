#!/bin/bash
# #410: Ajustes e shader reais em FBO/CGL, sem WindowServer nem TV Samsung.
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-vidro410.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-O1 -g -Isrc -I/usr/include -I/usr/include/SDL2
       -Wno-deprecated-declarations -Wno-macro-redefined)
rc=0
for plataforma in host tpk; do
  target=()
  [ "$plataforma" != tpk ] || target=(-DNV410_TPK)
  # Apenas Ajustes recebe NV_TPK; o backend grafico continua CGL do host.
  # dead_strip descarta integracoes Tizen que este teste nao executa.
  cc "${flags[@]}" "${target[@]}" -c tests/vidro410.c -o "$work/ajustes.o"
  cc "${sources[@]}" "$work/ajustes.o" "${flags[@]}" -o "$work/teste" \
    -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl -ffunction-sections -fdata-sections -Wl,--gc-sections
  NUVIO_DADOS="$work" "$work/teste" || rc=1
done
exit "$rc"
