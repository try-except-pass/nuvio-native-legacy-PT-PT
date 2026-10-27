#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Exercita o scheduler real de src/app.c com resolver Stalker bloqueavel.
set -eu
cd "$(dirname "$0")/.."
cc src/cwordem.c src/cwretido.c src/fonteantecipa.c tests/stalker_scheduler.c -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -o ${TMPDIR:-/tmp}/nuvio-stalker-scheduler -O0 -g -Wall -Wextra \
  -Wno-deprecated-declarations -ffunction-sections -fdata-sections \
  -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image -lSDL2_ttf -lz \
  -lGLESv2 -lEGL -lm -lpthread -ldl -pthread
timeout 5 ${TMPDIR:-/tmp}/nuvio-stalker-scheduler
