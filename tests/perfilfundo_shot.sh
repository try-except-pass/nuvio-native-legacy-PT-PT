#!/bin/bash
# Offline picker art regression. Optional historical GPU implementation for A/B proof.
set -euo pipefail
cd "$(dirname "$0")/.."
out=${NUVIO_PERFILFUNDO_OUT:-${TMPDIR:-/tmp}/rev/perfilfundo/after}
mkdir -p "$out"
bin=$(mktemp -d)
trap 'rm -rf "$bin"' EXIT
sources=()
flags=()
for f in src/*.c src/dts/*.c; do
  [ "$f" = src/main.c ] && continue
  if [ "$f" = src/gpunivel.c ] && [ -n "${NUVIO_PERFILFUNDO_GPU_REV:-}" ]; then
    git show "$NUVIO_PERFILFUNDO_GPU_REV:src/gpunivel.c" > "$bin/gpunivel.c"
    sources+=("$bin/gpunivel.c")
    flags+=(-DNV_BEFORE_GPUAUTO)
  else
    sources+=("$f")
  fi
done
cc "${sources[@]}" tests/perfilsel_visual.c "${flags[@]}" -DNV_PERFILSEL_TEST -Isrc \
  -o "$bin/shot" -O1 -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl \
  -Wno-deprecated-declarations -Wno-macro-redefined
mkdir "$bin/data"
rc=0
NUVIO_DADOS="$bin/data" NUVIO_TESTE_DIR="$bin/data" NUVIO_PERFILSEL_FUNDO=0 \
  NUVIO_PERFILSEL_SHOT_PREFIX="$out" NUVIO_PERFILFUNDO_TEST="${NUVIO_PERFILFUNDO_LEVEL:-auto}" \
  "$bin/shot" || rc=$?
ffmpeg -nostdin -loglevel error -y -i "$out-picker.bmp" "$out-picker.png" >/dev/null
rm "$out-picker.bmp"
exit "$rc"
