#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Capturas do cartão 1.5.1: cenas PT/EN/reduzidas e sequência opcional PNG.
# GL oculto com FBO 1920x1080; a sequência tem 240 quadros simulados a 12 fps.
#   bash tests/novidades151_shot.sh /tmp/nuvio-whatsnew151
set -eu
cd "$(dirname "$0")/.."
out=${1:-${TMPDIR:-/tmp}/nuvio-whatsnew151}
mkdir -p "$out"
NUVIO_DADOS=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-n151-shot.XXXXXX)
export NUVIO_DADOS
trap 'rm -rf "$NUVIO_DADOS"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in */main.c) continue;; esac
  sources+=("$source")
done
cc "${sources[@]}" tests/novidades151_shot.c -Isrc \
  -o ${TMPDIR:-/tmp}/nuvio-n151-shot -O1 -g \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wall -Wextra -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-n151-shot "$out"
echo "Imagens: $out"
echo "Vídeo opcional: ffmpeg -framerate 12 -i '$out/frame-%04d.png' -c:v libx264 -pix_fmt yuv420p ${TMPDIR:-/tmp}/nuvio-whatsnew151.mp4"
