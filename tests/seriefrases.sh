#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Frases (Wikiquote) e ficha de producao (Wikidata): leitura da resposta SPARQL,
# limpeza de wikitexto, extracao das falas nos dois formatos, e o cache — com o
# CACHE NEGATIVO, que e o que segura o custo dos titulos que nao tem nada.
#
#   bash tests/seriefrases.sh
#
# Compila tudo MENOS src/seriefrases.c, que o teste inclui.
#
# NUVIO_DADOS APONTA PARA UMA PASTA TEMPORARIA, e o proprio teste se recusa a
# rodar se dados_dir() nao for ela.
set -eu
cd "$(dirname "$0")/.."

NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-seriefrases-dados.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
echo "dados do teste em $NUVIO_DADOS"

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/seriefrases.c) continue;; esac
  sources+=("$source")
done

cc "${sources[@]}" tests/seriefrases.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-seriefrases \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-seriefrases
