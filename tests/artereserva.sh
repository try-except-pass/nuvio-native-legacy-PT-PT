#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Reserva de arte pelo TMDB. Sem rede: o teste substitui rede_baixar e a chave.
#
#   bash tests/artereserva.sh
set -eu
cd "$(dirname "$0")/.."
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc ${flags[@]+"${flags[@]}"} src/artereserva.c src/artefontes.c src/artetamanho.c src/js.c src/redeurl.c tests/artereserva.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o ${TMPDIR:-/tmp}/nuvio-artereserva-tests -O1 -g -Wno-macro-redefined \
  -Wall -Wextra -Wno-deprecated-declarations
${TMPDIR:-/tmp}/nuvio-artereserva-tests
