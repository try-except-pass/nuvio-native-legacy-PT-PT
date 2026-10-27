#!/bin/bash
# Plugin ordering, round accounting and shared empty-source message; no network.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-plugins-fila.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-DNV_WEBOS -DPLUG_TETO_MS=150 -std=gnu11 -O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 -pthread -ffunction-sections -fdata-sections -Wno-macro-redefined)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
cc "${flags[@]}" tests/plugins_fila.c src/js.c src/rede.c -o "$work/t"
"$work/t"
python3 tools/idiomas.py
