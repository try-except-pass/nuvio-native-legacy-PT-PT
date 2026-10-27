#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
quota_dir=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-seekrquota-pipeline.XXXXXX)
trap 'rm -rf "$quota_dir"' EXIT
cc src/seekrquota.c src/dados.c src/seekrvtt.c src/js.c src/jpegrapido.c \
  tests/seekrquota_pipeline.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lGLESv2 -lEGL -lm -lpthread -ldl -pthread -O1 -g \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined \
  -o ${TMPDIR:-/tmp}/nuvio-seekrquota-pipeline
NUVIO_DADOS="$quota_dir" ${TMPDIR:-/tmp}/nuvio-seekrquota-pipeline
