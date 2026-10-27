#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Sem rede nem tela: qual fonte de trailer cada ajuste tenta, nas duas TVs.
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -Wall -Isrc -I/usr/include tests/trailer-fonte.c src/trailerfonte.c -o ${TMPDIR:-/tmp}/nuvio-trailer-fonte
${TMPDIR:-/tmp}/nuvio-trailer-fonte
