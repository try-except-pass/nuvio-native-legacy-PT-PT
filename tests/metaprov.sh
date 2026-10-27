#!/bin/bash
# metaprov: URLs do catalogo do Nuvio (idioma, ids, busca) e decisao de reserva
# para o Cinemeta, com rede falsa.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-metaprov.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -std=gnu11 -Wall -Wextra -pthread ${NV_CFLAGS:--I/usr/include -I/usr/include/SDL2} -Isrc tests/metaprov.c src/metaprov.c -o "$work/test"
"$work/test"
