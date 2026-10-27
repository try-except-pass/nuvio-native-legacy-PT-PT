#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Captura do pouso vertical entre fileiras (issue #205), sem rede (artes de
# deploy/app/art). Nao entra na suite (testa-tudo.sh pula *_shot.sh): precisa
# de janela GL e de olho humano.
#
#   bash tests/vertfileira_shot.sh /tmp/nv-vertfileira
#
# Cada BMP vira PNG (ffmpeg) e o BMP e apagado.
set -eu
cd "$(dirname "$0")/.."

saida="${1:-${TMPDIR:-/tmp}/nv-vertfileira}"
mkdir -p "$saida"
NUVIO_DADOS=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-vertfileira-shot.XXXXXX")
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT

sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c) continue;; esac
  sources+=("$source")
done
binario="${NV_BINARY:-$NUVIO_DADOS/shot}"
if [ -z "${NV_REUSE:-}" ]; then
if ! cc "${sources[@]}" tests/vertfileira_shot.c -Isrc -o "$binario" \
  -O1 -g ${NV_CFLAGS:-} -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined \
  >"$NUVIO_DADOS/build.log" 2>&1; then
  cat "$NUVIO_DADOS/build.log" >&2
  exit 1
fi
grep -E "vertfileira_shot|error" "$NUVIO_DADOS/build.log" || true
fi
"$binario" "$saida/v"
for f in "$saida"/v-*.bmp; do
  [ -e "$f" ] || continue
  ffmpeg -nostdin -loglevel error -y -i "$f" "${f%.bmp}.png" >/dev/null 2>&1 && rm -f "$f"
done
