#!/bin/bash
# Home hero synopsis, Normal x Grande, pt/de/ru, BMP at 1080p. Not in the suite (needs a GL window).
#   bash tests/sinopse_shot.sh ${TMPDIR:-/tmp}/rev/sinopse
set -eu
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d /tmp/nuvio-sinopse-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT

ENV_D=$(tools/env.sh --allow-unconfigured 2>/dev/null || true)
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
eval cc -DNV_SHOT_HOOKS '"${sources[@]}"' tests/sinopse_shot.c -Isrc -o /tmp/nuvio-sinopse-shot \
  -O1 -g "$ENV_D" \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
/tmp/nuvio-sinopse-shot "$NUVIO_DADOS" "$@"
