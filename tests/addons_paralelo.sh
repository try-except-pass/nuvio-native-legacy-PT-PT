#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-addons-par.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/addons_paralelo.c src/linguas.c src/js.c src/legextras.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -std=gnu11 -pthread -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -Wall -Wextra -Wno-unused-function -Wno-macro-redefined -o "$work/t"
"$work/t"
