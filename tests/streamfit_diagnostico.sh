#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-streamfit-diagnostic.XXXXXX")
srv=""
trap '[ -z "$srv" ] || kill "$srv" 2>/dev/null || true; rm -rf "$tmp"' EXIT
python3 tests/streamfit_diagnostico_server.py "$tmp/port" &
srv=$!
for i in $(seq 50); do [ -s "$tmp/port" ] && break; sleep .1; done
test -s "$tmp/port"
flags=(-std=gnu11 -O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 -pthread -ffunction-sections -fdata-sections -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-Wl,--gc-sections -ldl); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
if [ "${SANITIZE:-0}" = thread ]; then flags+=(-fsanitize=thread -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/streamfit_diagnostico.c src/streamfitdiag.c src/redemarca.c src/streamfit.c src/vazao.c src/rede.c src/redeurl.c -o "$tmp/test"
"$tmp/test" "$(cat "$tmp/port")"
