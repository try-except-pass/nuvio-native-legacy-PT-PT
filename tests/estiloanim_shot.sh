#!/bin/bash
# Abertura do seletor Estilo da fileira (estiloanim_shot.c), em PNG,
# com a fonte da TV por padrao (NUVIO_SHOT_FONTE=3). Janela GL escondida do
# Mac; sem rede.
#
#   bash tests/estiloanim_shot.sh [pasta]
set -eu
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/nv-estiloanim-shots}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-estiloanim-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
if ! cc "${sources[@]}" tests/estiloanim_shot.c -Isrc -o "$NUVIO_DADOS/shot" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  ${NV_SHOT_CFLAGS:-} -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined \
  >"$NUVIO_DADOS/build.log" 2>&1; then
  cat "$NUVIO_DADOS/build.log" >&2
  exit 1
fi
grep -E "ctxinfo|ctxmenu|error" "$NUVIO_DADOS/build.log" || true
NUVIO_SHOT_FONTE="${NUVIO_SHOT_FONTE:-3}" "$NUVIO_DADOS/shot" "$saida"
