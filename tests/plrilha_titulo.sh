#!/bin/bash
# Constantes do titulo delicado da Central (sem GL). bash tests/plrilha_titulo.sh
set -eu
cd "$(dirname "$0")/.."
w=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-plrt.XXXXXX"); trap 'rm -rf "$w"' EXIT
cc tests/plrilha_titulo.c -Isrc -I/usr/include -I/usr/include/SDL2 -o "$w/t" -lm -Wno-macro-redefined
"$w/t"
