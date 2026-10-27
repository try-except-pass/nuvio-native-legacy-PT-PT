#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
cc tests/sidebaratv_geometry.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  -Wl,--unresolved-symbols=ignore-all -o ${TMPDIR:-/tmp}/nuvio-sidebaratv-geometry
${TMPDIR:-/tmp}/nuvio-sidebaratv-geometry
