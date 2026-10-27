#!/bin/bash
# Capturas do grafico de temporadas ("Seu progresso") na pagina de serie, em
# PNG, sem rede. Nao entra na suite (testa-tudo.sh pula *_shot.sh): precisa de
# janela GL e de olho humano.
#
#   bash tests/temporadas_grafico_shot.sh /tmp/nuvio-tgraf
#   NUVIO_SHOT_FONTE=3 bash tests/temporadas_grafico_shot.sh /tmp/nuvio-tgraf
#
# Compila tudo MENOS src/main.c, src/detail.c e src/temporadas_grafico.c, que a
# captura inclui (ela troca a agenda e o indice social antes do include).
set -eu
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-tgraf-dados.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
bin="${TMPDIR:-/tmp}/nuvio-tgraf-shot"

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/detail.c|src/temporadas_grafico.c) continue;; esac
  sources+=("$source")
done

cc "${sources[@]}" tests/temporadas_grafico_shot.c -Isrc -o "$bin" \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
"$bin" "$@"
