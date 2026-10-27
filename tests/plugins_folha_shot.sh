#!/bin/bash
# Folha de fontes via manifesto -> QuickJS -> addons. Servidor local por padrao.
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/plugins_folha-shot.XXXXXX")
NUVIO_DADOS="$tmp/dados"; mkdir -p "$NUVIO_DADOS"; export NUVIO_DADOS
srv=""
trap '[ -z "$srv" ] || kill "$srv" 2>/dev/null || true; rm -rf "$tmp"' EXIT
if [ -z "${PLUG_REPO:-}" ]; then
  python3 tests/plugins_server.py "$tmp/porta" &
  srv=$!
  for i in $(seq 300); do [ -s "$tmp/porta" ] && break; kill -0 "$srv"; sleep 0.1; done
  [ -s "$tmp/porta" ]
  export PLUG_REPO="http://127.0.0.1:$(<"$tmp/porta")/manifest.json"
fi
# PLUG_REPO points to a real repository when an external capture is wanted.
sources=()
for source in src/*.c src/dts/*.c; do case "$source" in src/main.c) continue;; esac; sources+=("$source"); done
cc "${sources[@]}" tests/plugins_folha_shot.c -Isrc -o "$tmp/shot" -O1 -g \
  -I/usr/include -I/usr/include/SDL2 -L/usr/lib/x86_64-linux-gnu -lSDL2 -lSDL2_image \
  -lSDL2_ttf -lz -lGLESv2 -lEGL -lm -lpthread -ldl -Wno-deprecated-declarations -Wno-macro-redefined
"$tmp/shot" "$@"
