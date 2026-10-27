#!/bin/bash
# Pre-busca dos vizinhos do carrossel (2.0.3). Ver tests/carrossel_prebusca.c.
set -euo pipefail
cd "$(dirname "$0")/.."
NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-carrossel-prebusca.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/carrossel_prebusca.c -Isrc -o "${TMPDIR:-/tmp}/nuvio-carrossel-prebusca-tests" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
"${TMPDIR:-/tmp}/nuvio-carrossel-prebusca-tests"
echo "PASS: vizinhos do carrossel pre-buscados; voltar a titulo visto nao pede"
