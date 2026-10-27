#!/bin/bash
# Isolated real-GL regression proof; optional output directory holds PNG sheets.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-salvostabs.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
out="${1:-$tmp/pngs}"
mkdir -p "$out"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/salvospainel.c) continue;; esac
  sources+=("$source")
done
proof_source=()
if [ -n "${SALVOSTABS_BASELINE:-}" ]; then
  git show "$SALVOSTABS_BASELINE:src/salvospainel.c" > "$tmp/salvospainel.c"
  proof_source=(-DSALVOSTABS_SOURCE=\"$tmp/salvospainel.c\")
fi
cc "${sources[@]}" tests/salvostabs.c "${proof_source[@]}" -Isrc -o "$tmp/test" \
  -DNV_REC_URL='"http://127.0.0.1:8799"' -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir -p "$tmp/data"
NUVIO_DADOS="$tmp/data" "$tmp/test" "$out"
