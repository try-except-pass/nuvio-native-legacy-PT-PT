#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -euo pipefail
cd "$(dirname "$0")/.."
binary=$(mktemp ${TMPDIR:-/tmp}/nuvio-diag-persistencia.XXXXXX)
trap 'rm -f "$binary"' EXIT
# Dead-strip uncalled UI/network paths; no SDL runtime or whole-core build.
case "$(uname -s)" in
  Darwin) stripflag=-ffunction-sections -fdata-sections -Wl,--gc-sections ;;
  *) stripflag=-Wl,--gc-sections ;;
esac
cc tests/diagnostico_persistencia.c src/perfiltv.c -Isrc -DNV_ANDROID \
  -O1 -g -pthread -ffunction-sections -fdata-sections "$stripflag" \
  -I/usr/include -I/usr/include/SDL2 \
  -Wno-deprecated-declarations -Wno-macro-redefined -o "$binary"
"$binary"
