#!/bin/bash
# PNG proof for the profile default collection view and a collection following it.
set -eu
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/rev/colecao-padrao-png}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-colexib-proof.XXXXXX")
export NUVIO_DADOS
bin="${TMPDIR:-/tmp}/nuvio-colexib-proof"
trap 'rm -rf "$NUVIO_DADOS" "$bin"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/vertudo.c) continue;; esac
  sources+=("$source")
done
cc -DAJUSTES_TESTE "${sources[@]}" tests/colecao_padrao_proof.c -Isrc -o "$bin" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
"$bin" "$saida"
for f in "$saida"/*.bmp; do
  [ -e "$f" ] || continue
  ffmpeg -nostdin -loglevel error -y -i "$f" "${f%.bmp}.png" >/dev/null 2>&1 && rm -f "$f"
done
