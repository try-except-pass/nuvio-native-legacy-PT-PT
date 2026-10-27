#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc tests/livetvdiag_limitada.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -O1 -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -lm -lpthread -o "$work/test"
"$work/test"
