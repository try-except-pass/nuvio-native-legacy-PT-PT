#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas do painel "Carregamento da Home" da ilha. Nao entra na suite.
#   bash tests/homecarga_painel_shot.sh [prefixo]      (ANTES=<dir src antigo>: o painel antigo)
set -eu
cd "$(dirname "$0")/.."
export TMPDIR
NUVIO_DADOS=$(mktemp -d "$TMPDIR/nuvio-hcp-dados.XXXXXX"); export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
S=${ANTES_SRC:-src}; extra=(); [ -n "${ANTES_SRC:-}" ] && extra+=(-DANTES)
sources=(); for s in $S/*.c; do [ "$s" != "$S/main.c" ] && sources+=("$s"); done
# DTS sources added in 2a97f7e7; older ANTES_SRC trees may lack this directory.
if [ -d "$S/dts" ]; then sources+=("$S"/dts/*.c); fi
cc "${sources[@]}" tests/homecarga_painel_shot.c -I$S -o "$TMPDIR/homecarga-painel-shot" \
  -O1 -g -DNV_LEVE ${extra[@]+"${extra[@]}"} -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"$TMPDIR/homecarga-painel-shot" "${1:-${TMPDIR:-/tmp}/nuvio-homecarga-painel}"
