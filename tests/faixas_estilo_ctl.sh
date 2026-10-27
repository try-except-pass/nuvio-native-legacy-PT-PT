#!/bin/bash
# Estilo tab controls. See tests/faixas_estilo_ctl.c.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-faixas-canal.XXXXXX")
trap 'rm -rf "$work"' EXIT
linker=-Wl,--gc-sections
if [ "$(uname -s)" = Darwin ]; then linker=-ffunction-sections -fdata-sections -Wl,--gc-sections; fi
cc tests/faixas_estilo_ctl.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -std=gnu11 -ffunction-sections -fdata-sections "$linker" \
  -Wall -Wextra -Wno-unused-function -Wno-macro-redefined -o "$work/t" -lm
"$work/t"
