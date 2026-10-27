#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas do Spotlight (src/spotlight.c) para revisao visual, com as
# conferencias de comportamento do .c (filtro, pedido, recentes, Voltar).
#
#   bash tests/spotlight_shot.sh /tmp/nuvio-spotlight
#
# Nao entra na suite (tools/testa-tudo.sh pula *_shot.sh): precisa de janela GL.
set -eu
cd "$(dirname "$0")/.."
saida="${1:-${TMPDIR:-/tmp}/nuvio-spotlight}"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-spot-shot-XXXXXX")"
dados="$(mktemp -d "${TMPDIR:-/tmp}/nuvio-spot-dados-XXXXXX")"
# addon de canais falso (o mesmo de spotlight_canais.sh) para a captura -canal
python3 tests/spotlight_canais_servidor.py 8767 2>/dev/null & SRV=$!
trap 'kill $SRV 2>/dev/null || true; rm -rf "$tmp" "$dados"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" = "src/main.c" ] && continue
  sources+=("$source")
done
cc "${sources[@]}" tests/spotlight_shot.c -Isrc -o "$tmp/shot" -O1 -g \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$dados" SPOT_CANAIS_BASE="http://127.0.0.1:8767" "$tmp/shot" "$saida"
if command -v ffmpeg >/dev/null 2>&1; then
  for b in "$saida"-*.bmp; do
    ffmpeg -nostdin -loglevel error -y -i "$b" -vf scale=1400:1400:force_original_aspect_ratio=decrease "${b%.bmp}.jpg" >/dev/null
  done
fi
