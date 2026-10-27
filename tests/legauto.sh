#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-legauto.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/legauto.c -Isrc -I/usr/include -I/usr/include/SDL2 -std=gnu11 -g -Wall -Wno-unused-function -Wno-macro-redefined -o "$work/t"
"$work/t"
cc tests/legauto_fonte_guardada.c src/linguas.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -std=gnu11 -g -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -Wall -Wextra -Wno-unused-function -Wno-macro-redefined -o "$work/fonte"
"$work/fonte"
