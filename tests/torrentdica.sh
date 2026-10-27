#!/bin/bash
# Real sources-sheet rendering, with isolated data and no network requests.
set -euo pipefail
cd "$(dirname "$0")/.."
D=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-torrentdica.XXXXXX")
trap 'rm -rf "$D"' EXIT
out="${1:-${TMPDIR:-/tmp}/rev/torrentdica/after.png}"
mkdir -p "$(dirname "$out")" "$D/data"
flags=(-O0 -g -Isrc -I/usr/include -I/usr/include/SDL2 -Wno-deprecated-declarations -Wno-macro-redefined)
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" = src/main.c ] || [ "$source" = src/streams.c ] || sources+=("$source")
done
printf '%s\n' "${sources[@]}" tests/torrentdica.c |
  xargs -P 8 -I{} sh -c 'cc "$@" -c {} -o "$0/$(echo {} | tr / _).o"' "$D" "${flags[@]}"
cc "$D"/*.o -o "$D/test" -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
NUVIO_DADOS="$D/data" "$D/test" "$out"
# Only the included streams.c needs the browser define; keep the native GL harness.
cc "${flags[@]}" -D__EMSCRIPTEN__ -c tests/torrentdica.c -o "$D/tests_torrentdica.c.o"
cc "$D"/*.o -o "$D/test_wgt" -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl
rm -f "$D/data/debrid.txt"
NUVIO_DADOS="$D/data" "$D/test_wgt" "${out%.png}-wgt.png"
