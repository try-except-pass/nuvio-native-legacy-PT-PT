#!/bin/bash
# Build the real application modules; first/repeat use the same isolated disk.
# Shot mode follows the existing hidden SDL/GL FBO harness, no account/network.
set -euo pipefail
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/tmp}"
mkdir -p "$TMPDIR"
dados=$(mktemp -d "$TMPDIR/nuvio-dicascontrole.XXXXXX")
export NUVIO_DADOS="$dados"
trap 'rm -rf "$dados"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
cc -DAJUSTES_TESTE -DNV_TOUCH_UI "${sources[@]}" tests/dicascontrole.c -Isrc -o "$dados/test" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
cc -DNV_TOUCH_UI -fsyntax-only src/main.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$dados/test" "$dados" first
"$dados/test" "$dados" repeat
if [ "${1:-}" = shot ]; then
  saida="${2:-${TMPDIR:-/tmp}/rev/dicascontrole}"
  mkdir -p "$saida"
  "$dados/test" "$dados" shot "$saida"
fi
