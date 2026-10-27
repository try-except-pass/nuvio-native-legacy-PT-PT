#!/bin/bash
# B4: o mesmo addon em variantes de URL vira UM na lista. Ver o cabecalho de
# tests/addons_duplicados.c. NAO precisa de rede.
#
#   bash tests/addons_duplicados.sh
set -eu
cd "$(dirname "$0")/.."
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
       -I/usr/include -I/usr/include/SDL2 \
       -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
bin="${TMPDIR:-/tmp}/nuvio-addons-duplicados-tests"
# SO addons.c, como tests/addons_diferenca.sh.
cc "${flags[@]}" src/addons.c tests/addons_duplicados.c -o "$bin"
saida=$("$bin") || { echo "$saida" | grep -v "^\[addons\]"; echo "addons_duplicados.sh: FALHOU"; exit 1; }
echo "$saida" | grep -v '^\[addons\]' || true
echo "$saida" | grep -q 'addons_duplicados: ok'
echo "addons_duplicados.sh: ok"
