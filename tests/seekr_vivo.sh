#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Seekr contra a API real (precisa de rede). Ver tests/seekr_vivo.c.
set -eu
cd "$(dirname "$0")/.."
cc src/seekr.c src/seekrquota.c src/dados.c src/seekrvtt.c src/js.c src/rede.c src/redeurl.c src/jpegrapido.c tests/seekr_vivo.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lGLESv2 -lEGL -lm -lpthread -ldl -o ${TMPDIR:-/tmp}/nuvio-seekr-vivo -O1 -g \
  -Wno-deprecated-declarations -Wno-macro-redefined
quota_dir=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-seekr-vivo.XXXXXX)
trap 'rm -rf "$quota_dir"' EXIT
NUVIO_DADOS="$quota_dir" ${TMPDIR:-/tmp}/nuvio-seekr-vivo
