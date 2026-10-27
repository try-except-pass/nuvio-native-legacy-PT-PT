#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-guia-busca-step.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-DNV_ANDROID -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
backup_extra=()
if [ -f src/backupcategories.c ]; then backup_extra+=(src/backupcategories.c); fi
cc ${backup_extra[@]+"${backup_extra[@]}"} tests/guiabusca_incremental.c src/ltv.c -Isrc -I/usr/include -I/usr/include/SDL2 -O1 -g "${flags[@]}" -lpthread -lm -o "$work/test"
"$work/test"
