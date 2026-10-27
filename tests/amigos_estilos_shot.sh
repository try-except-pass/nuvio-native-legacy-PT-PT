#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/rev/amigosestilo/depois}"
mkdir -p "$saida"
export NUVIO_DADOS
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-abas-shot.XXXXXX")
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/recomenda.c|src/rede.c|src/celular.c) continue;; esac
  if [ "${NV_AMIGOS_BEFORE:-0}" = 1 ] && [ "$source" = src/amigosfil.c ]; then
    git show d626daf5c:src/amigosfil.c > "$NUVIO_DADOS/amigosfil-before.c"
    sources+=("$NUVIO_DADOS/amigosfil-before.c")
  else
    sources+=("$source")
  fi
done
flags=()
if [ "${NV_AMIGOS_BEFORE:-0}" != 1 ]; then flags+=(-DNV_AMIGOSFIL_TESTE); fi
cc "${sources[@]}" tests/amigos_estilos_shot.c -Isrc -o "$NUVIO_DADOS/shot"   -O1 -g -DNV_SOCIALVIS_DEMO "${flags[@]}" -I/usr/include -I/usr/include/SDL2   -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl   -Wno-deprecated-declarations -Wno-macro-redefined >"$saida/build.log" 2>&1 || { tail -60 "$saida/build.log"; exit 1; }
"$NUVIO_DADOS/shot" "$saida/s" >"$saida/test.log" 2>&1 || { tail -40 "$saida/test.log"; exit 1; }
for f in "$saida"/s-*.bmp; do
  [ -e "$f" ] || continue
  ffmpeg -nostdin -loglevel error -y -i "$f" "${f%.bmp}.png" >/dev/null 2>&1
  rm "$f"
done
echo "friends styles: ok"
