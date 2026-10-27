#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR}/nuvio-detail-layout.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -c src/descoberta.c -Drede_baixar=buscar_fixture -Isrc \
  -I/usr/include -I/usr/include/SDL2 -O1 -g -o "$work/descoberta.o"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c|src/descoberta.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" "$work/descoberta.o" tests/detail_layout.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-detail-layout-tests \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-detail-layout-tests
