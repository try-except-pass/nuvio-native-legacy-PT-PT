#!/bin/bash
# Row 30 (mapa-erros-logs): o medidor de quadro contava o tempo SUSPENSO
# (pior=4191 s, 881 s, 54 s, 30 s) e a triagem procurava jank que nao existia.
#   bash tests/quadroc.sh
set -euo pipefail
cd "$(dirname "$0")/.."
cc -O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 \
  -Wall -Wextra tests/quadroc.c -o "${TMPDIR:-/tmp}/quadroc-teste"
"${TMPDIR:-/tmp}/quadroc-teste"
rm -f "${TMPDIR:-/tmp}/quadroc-teste"
