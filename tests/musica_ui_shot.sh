#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root=${TMPDIR:-/tmp}/musica-visual
out=${TMPDIR:-/tmp}/rev/musica-impl-png
mkdir -p "$out"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-musica-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/musicaui.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/musica_ui_shot.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -DNV_MUSICA_SHOT -DNV_REC_URL='"https://rec.nuvio.app"' \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl -Wno-deprecated-declarations
"$NUVIO_DADOS/shot" "$root" "$out"
for mode in filme foco-faixa serie guardado; do
  magick "$root/musica-$mode.png" "$out/app-$mode.png" +append "$out/compare-$mode.png"
done
