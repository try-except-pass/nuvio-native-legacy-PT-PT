#!/bin/bash
# #378: legenda "Da conta" vira "Nenhuma" com os ajustes locais protegidos.
# Ver o cabecalho de tests/legconta378.c (a parte do sync.c esta em
# tests/syncordem.sh, sessao "legconta").
#
#   bash tests/legconta378.sh
set -euo pipefail
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-legconta378.XXXXXX")
trap 'rm -rf "$work"' EXIT
extra=()
if [ "${SANITIZE:-0}" = 1 ]; then extra+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${sources[@]}" tests/legconta378.c -Isrc -o "$work/t" ${extra[@]+"${extra[@]}"} \
  -O1 -g -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
saida=$(NV_T_DIR="$work" "$work/t")
printf '%s\n' "$saida"
# A linha do log diz o que a CONTA guarda e, a parte, o que esta valendo.
printf '%s\n' "$saida" | grep -qF 'idiomas da conta: legenda="en" legenda2="" audio=""; em vigor: legenda="es"' \
  || { echo "FALHOU: linha do log nao separa conta de em vigor"; exit 1; }
echo "legconta378.sh: ok"
