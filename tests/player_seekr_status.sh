#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -euo pipefail
cd "$(dirname "$0")/.."
binary=$(mktemp ${TMPDIR:-/tmp}/nuvio-player-seekr-status.XXXXXX)
trap 'rm -f "$binary"' EXIT
case "$(uname -s)" in
  Darwin) stripflag=-ffunction-sections -fdata-sections -Wl,--gc-sections ;;
  *) stripflag=-Wl,--gc-sections ;;
esac
cc tests/player_seekr_status.c src/seekr.c -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -O1 -g -ffunction-sections -fdata-sections "$stripflag" \
  -Wno-deprecated-declarations -Wno-macro-redefined -o "$binary"
"$binary"
