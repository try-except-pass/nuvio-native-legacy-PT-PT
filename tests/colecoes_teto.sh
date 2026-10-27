#!/bin/bash
# #255: 7 colecoes, 296 pastas, 598 fontes e ordem de 830 itens na Home real.
set -eu
cd "$(dirname "$0")/.."
flags=(-O1 -g -Isrc -pthread -I/usr/include -I/usr/include/SDL2 -ffunction-sections -fdata-sections -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
bin="$(mktemp "${TMPDIR:-/tmp}/nuvio-colecoes-teto.XXXXXXXX")"
trap 'rm -f "$bin"' EXIT
cc "${flags[@]}" src/catalogo.c src/progresso.c src/focus.c src/ajustes.c src/posterprov.c src/redeurl.c src/colecoes.c src/colfileiras.c src/js.c src/catordem.c src/fileiras.c src/artehero.c src/cwordem.c src/cwretido.c src/fonteregra.c src/regexjs.c src/ajlog.c tests/colecoes_teto.c tests/amigosfil_stub.c -ffunction-sections -fdata-sections -Wl,--gc-sections -o "$bin"
"$bin"
