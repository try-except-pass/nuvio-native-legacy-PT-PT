#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
binary=$(mktemp ${TMPDIR:-/tmp}/nuvio-icon-policy.XXXXXX)
trap 'rm -f "$binary"' EXIT
cc src/iconeapp.c tests/iconeapp_policy.c -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -o "$binary"
"$binary"
