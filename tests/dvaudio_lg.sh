#!/bin/bash
# A troca de TrueHD por E-AC-3 para manter o DV (dvPronto): a faixa ATUAL da TV
# e traduzida para a faixa do MKV por idioma/codec/canais antes de escolher a
# substituta. Compila o ramo LS2 real de src/video.c no host (como
# video_pausa_lg), com o barramento e o motor do DV dublados.
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-dvaudio-lg.XXXXXX")
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
cc "${flags[@]}" "${sources[@]}" tests/dvaudio_lg.c -o "$dir/test" \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
"$dir/test"
