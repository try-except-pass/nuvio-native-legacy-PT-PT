#!/bin/bash
# Selo de assistido: modo escrito desenha o rotulo; modo icone nao.
set -eu
cd "$(dirname "$0")/.."
cc -O1 -g -ffunction-sections -fdata-sections -Wl,--gc-sections -ffunction-sections -fdata-sections -Isrc -I/usr/include -I/usr/include/SDL2 \
  -Wno-macro-redefined -Wno-deprecated-declarations tests/selo_visto_modo.c -o /tmp/nuvio-selo-visto-modo
/tmp/nuvio-selo-visto-modo
