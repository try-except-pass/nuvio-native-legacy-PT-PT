#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -euo pipefail
cd "$(dirname "$0")/.."
binary=$(mktemp ${TMPDIR:-/tmp}/nuvio-seekr-estados.XXXXXX)
trap 'rm -f "$binary"' EXIT
case "$(uname -s)" in
  Darwin) stripflag=-ffunction-sections -fdata-sections -Wl,--gc-sections ;;
  *) stripflag=-Wl,--gc-sections ;;
esac
cc tests/seekr_estados.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -O1 -g -ffunction-sections -fdata-sections "$stripflag" \
  -Wno-deprecated-declarations -Wno-macro-redefined -o "$binary"
"$binary"
