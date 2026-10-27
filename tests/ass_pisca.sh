#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Pisca e atraso da legenda ASS no overlay (#92). Ver tests/ass_pisca.c.
# Precisa do libass do Homebrew; sem ele, pula.
set -eu
cd "$(dirname "$0")/.."
if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists libass; then
  echo "ass_pisca: libass ausente (teste ignorado)"; exit 0
fi
cc -DNV_ASS_LIBASS -include tests/ass_pisca_gl.h -Isrc tests/ass_pisca.c src/assrender.c tests/ass_render_stub.c \
  -o ${TMPDIR:-/tmp}/nuvio-ass-pisca-test \
  $(pkg-config --cflags --libs libass) -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lGLESv2 -lEGL -lm -lpthread -ldl -Wno-deprecated-declarations -Wno-macro-redefined
${TMPDIR:-/tmp}/nuvio-ass-pisca-test 2>/dev/null
