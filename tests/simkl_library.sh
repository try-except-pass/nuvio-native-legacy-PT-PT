#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
python3 tests/fixtures/simkl_library.py "$tmp/fixture.json"
cc tests/simkl_library.c tests/simkl_library_grid.c src/js.c -Isrc -I/usr/include -I/usr/include/SDL2 -O1 -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread -o "$tmp/test"
"$tmp/test" "$tmp/fixture.json"
