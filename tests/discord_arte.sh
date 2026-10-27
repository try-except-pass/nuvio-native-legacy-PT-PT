#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/discord-arte.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc -O1 -g -Isrc -I/usr/include -I/usr/include/SDL2 -pthread \
  tests/discord_arte.c src/js.c src/jsw.c -o "$work/discord-arte"
"$work/discord-arte"
