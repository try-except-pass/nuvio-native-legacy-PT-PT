#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-spot-reg-XXXXXX")"
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/dados"
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/spotlight.c) continue;; esac
  sources+=("$source")
done
flags=(-pthread)
if [ "${SANITIZE:-0}" = 1 ]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
fi
if [ -n "${SPOTLIGHT_SOURCE:-}" ]; then
  flags+=("-DSPOTLIGHT_SOURCE=\"$SPOTLIGHT_SOURCE\"")
fi
"${CC:-cc}" "${sources[@]}" tests/spotlight_regressao.c -Isrc -o "$tmp/t" -O1 -g \
  "${flags[@]}" \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$tmp/dados" SDL_AUDIODRIVER=dummy "$tmp/t"
