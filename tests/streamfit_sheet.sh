#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -euo pipefail
cd "$(dirname "$0")/.."
SF_DIR=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-streamfit-sheet.XXXXXX)
trap 'rm -rf "$SF_DIR"' EXIT
flags=(-O1 -g -Isrc -pthread -I/usr/include -I/usr/include/SDL2
       -Wall -Wextra -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
cc "${flags[@]}" tests/streamfit_sheet.c src/streamfit.c src/vazao.c src/badges.c src/fonteauto.c src/fontenota.c \
  src/fontefalha.c src/fontefiltro.c src/linguas.c src/limpa.c src/dobra.c -lm -o "$SF_DIR/test"
"$SF_DIR/test"
