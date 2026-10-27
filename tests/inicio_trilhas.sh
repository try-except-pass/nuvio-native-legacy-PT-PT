#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-inicio-trilhas.XXXXXX")
trap 'rm -rf "$dir"' EXIT
rc=0
for plataforma in NV_ANDROID NV_TPK NV_HOST; do
  cc -O1 -g -D"$plataforma" -Isrc -I/usr/include -I/usr/include/SDL2 \
    -Wno-deprecated-declarations -Wno-macro-redefined \
    -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
    src/inicio.c tests/inicio_trilhas.c -o "$dir/$plataforma"
  resultado=0
  "$dir/$plataforma" || resultado=$?
  echo "$plataforma rc=$resultado"
  if [ "$resultado" -ne 0 ]; then rc=1; fi
done
exit "$rc"
