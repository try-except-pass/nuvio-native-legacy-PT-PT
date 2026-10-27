#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-fontefalha-playback.XXXXXX")
trap 'rm -rf "$dir"' EXIT
flags=(-std=gnu11 -O1 -g -Isrc -pthread -Wall -Wextra -Wno-unused-function
       -ffunction-sections -fdata-sections -DNV_LINUX_DESKTOP)
if [ "$(uname -s)" = Darwin ]; then
  flags+=(-I/usr/include -I/usr/include/SDL2)
  linker=(-ffunction-sections -fdata-sections -Wl,--gc-sections)
else
  linker=(-Wl,--gc-sections)
  if pkg-config --exists sdl2; then
    read -r -a sdk <<< "$(pkg-config --cflags sdl2)"
    flags+=("${sdk[@]}")
  fi
fi
# CPU-only SDK type shims may be supplied; no SDL player/runtime is linked.
if [ -n "${CFLAGS:-}" ]; then read -r -a extra <<< "$CFLAGS"; flags+=("${extra[@]}"); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/fontefalha_playback.c src/fontefalha.c src/fontefiltro.c src/linguas.c src/fonteantecipa.c \
  src/inicio.c src/fontesonda.c \
  "${linker[@]}" -lm -o "$dir/teste"
"$dir/teste" "$dir"
