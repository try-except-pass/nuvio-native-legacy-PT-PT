#!/bin/bash
# O card de "Continuar assistindo" mostra o still do episodio quando
# "Miniatura do episodio" esta ligada (relato da Shield, teste 318.3).
# Ver tests/cwthumb_home.c.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-cwthumb.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc src/catalogo.c src/progresso.c src/focus.c src/ajustes.c src/ajlog.c src/posterprov.c \
  src/redeurl.c src/colecoes.c src/js.c src/metaprov.c src/catordem.c src/fileiras.c \
  src/artehero.c src/cwordem.c src/cwretido.c src/fonteregra.c src/regexjs.c \
  tests/cwthumb_home.c tests/amigosfil_stub.c \
  -Isrc -o "$work/test" -O1 -g -ffunction-sections -fdata-sections \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -Wno-deprecated-declarations -Wno-macro-redefined
"$work/test"
