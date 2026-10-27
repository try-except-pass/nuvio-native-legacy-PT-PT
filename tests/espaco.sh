#!/bin/bash
# A tecla de espaco da Busca e do Spotlight: digitar "the office" da "the
# office" (D-pad e teclado fisico) e o rotulo e a BARRA em todo idioma, nao o
# tema "espaco sideral" do mapa (tests/espaco.c).
#
#   bash tests/espaco.sh
set -eu
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-espaco.XXXXXX")
export NUVIO_DADOS
bin="$NUVIO_DADOS.bin"
trap 'rm -rf "$NUVIO_DADOS" "$bin"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/espaco.c -Isrc -o "$bin" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
if [ "$#" -gt 0 ]; then
  "$bin" "$1"
else
  "$bin"
  "$bin" spacing
  "$bin" flow-i18n
fi
