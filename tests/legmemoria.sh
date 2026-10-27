#!/bin/bash
# A legenda escolhida a mao volta (2.0.3). Ver tests/legmemoria.c.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-legmemoria.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/legmemoria.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -std=gnu11 -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -Wall -Wextra -Wno-unused-function -Wno-macro-redefined -Wno-missing-field-initializers -o "$work/t"
saida=$("$work/t")
printf '%s\n' "$saida"
# A linha que faltou no log da TCL.
printf '%s\n' "$saida" | grep -q "\[legenda\] automatica: ultima escolha 'pob' -> embutida 2"
printf '%s\n' "$saida" | grep -q "\[legenda\] automatica: ultima escolha 'none' -> nenhuma"
