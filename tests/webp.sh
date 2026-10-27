#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
san=()
if [ "${SANITIZE:-0}" = 1 ]; then san=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc -O1 -g -Wall -Wextra -pthread "${san[@]}" -Isrc -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu -lSDL2 -Wno-deprecated-declarations \
  src/webp.c tests/webp.c -o ${TMPDIR:-/tmp}/nuvio-webp-tests
${TMPDIR:-/tmp}/nuvio-webp-tests
