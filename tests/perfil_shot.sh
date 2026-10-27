#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas da tela "Perfil e Stats", em BMP, sem rede e sem conta do Trakt.
# NAO entra na suite: precisa de janela GL e de olho humano para julgar — mesma
# regra de tests/ajustes_shot.sh e tests/perfilsel.sh --capturas.
#
#   bash tests/perfil_shot.sh [prefixo]      (padrao: /tmp/nuvio-perfil)
set -eu
cd "$(dirname "$0")/.."
fontes=()
for f in src/*.c src/dts/*.c; do [ "$f" != src/main.c ] && fontes+=("$f"); done
cc "${fontes[@]}" tests/perfil_shot.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-perfil-shot \
  -O1 -g -DNV_SOCIALVIS_DEMO -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined

# PASTA DESCARTAVEL. NUVIO_DADOS e a unica variavel que dados_iniciar()
# respeita; NUVIO_TESTE_DIR e o que o binario confere antes de desenhar. Sem as
# duas ele se recusa a rodar, e e de proposito: um teste grafico ja apagou o
# ~/.nuvio de quem o rodou uma vez.
D=$(mktemp -d); trap 'rm -rf "$D"' EXIT
NUVIO_DADOS="$D" NUVIO_TESTE_DIR="$D" ${TMPDIR:-/tmp}/nuvio-perfil-shot "${1:-${TMPDIR:-/tmp}/nuvio-perfil}"
