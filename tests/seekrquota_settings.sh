#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
quota_dir=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-seekrquota-settings.XXXXXX)
trap 'rm -rf "$quota_dir"' EXIT
# --unresolved-symbols=ignore-all: o ajustes.c inteiro entra e o dead_strip nao
# corta tudo o que as telas novas (#311, #312) alcancam; o que a parte do Seekr
# nao chama fica sem resolver em vez de pedir dezenas de dubles.
cc src/dados.c tests/seekrquota_settings.c -Isrc -I/usr/include \
  -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu -lSDL2 \
  -O1 -g -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all \
  -Wno-deprecated-declarations -Wno-macro-redefined -o ${TMPDIR:-/tmp}/nuvio-seekrquota-settings
NUVIO_DADOS="$quota_dir" ${TMPDIR:-/tmp}/nuvio-seekrquota-settings
