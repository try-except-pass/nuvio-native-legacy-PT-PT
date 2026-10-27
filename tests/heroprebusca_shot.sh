#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Mac: pre-busca do destaque e sinopse dos candidatos (tests/heroprebusca_shot.c).
#
# As chaves do TMDB e do Trakt saem do local.properties do app web pelo
# tools/env.sh, como em tools/mac.sh — sem elas as fontes virtuais falham e a
# captura mostra a reserva (o que tambem e um caso valido de olhar).
#
#   bash tests/heroprebusca_shot.sh [segundos]
set -eu
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-heroprebusca-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT

ENV_D=$(tools/env.sh --allow-unconfigured 2>/dev/null || true)
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
eval cc '"${sources[@]}"' tests/heroprebusca_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-heroprebusca-shot \
  -O1 -g "$ENV_D" \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-heroprebusca-shot "$NUVIO_DADOS" "$@"
