#!/bin/bash
# A contagem do botao "Sincronizar addons" (#360). Ver tests/addons_diferenca.c.
# NAO precisa de rede.
#
#   bash tests/addons_diferenca.sh
set -eu
cd "$(dirname "$0")/.."
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
       -I/usr/include -I/usr/include/SDL2 \
       -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
bin="${TMPDIR:-/tmp}/nuvio-addons-diferenca-tests"
# SO addons.c, como tests/addonslista.sh: -dead_strip descarta o resto do
# modulo, e o que sobra (definir/exportar/diferenca) nao toca rede nem SDL.
cc "${flags[@]}" src/addons.c tests/addons_diferenca.c -o "$bin"
saida=$("$bin")
echo "$saida" | grep -v '^\[addons\]' || true
echo "$saida" | grep -q 'addons_diferenca: ok'
echo "addons_diferenca.sh: ok"
