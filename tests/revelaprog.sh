#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Barra de progresso animada e onda de grade (tests/revelaprog.c).
#
#   bash tests/revelaprog.sh
set -eu
cd "$(dirname "$0")/.."
cc tests/revelaprog.c -Isrc -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lm -o ${TMPDIR:-/tmp}/nuvio-revelaprog-tests -O1 -g -Wall -Wextra
${TMPDIR:-/tmp}/nuvio-revelaprog-tests
