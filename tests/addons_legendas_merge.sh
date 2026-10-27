#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-subtitles-merge.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc tests/addons_legendas_merge.c tests/addons_legendas_prazo.c src/linguas.c src/js.c src/legextras.c -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -std=gnu11 -pthread -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -Wall -Wextra \
  -Wno-unused-function -Wno-macro-redefined -Wno-missing-field-initializers -o "$work/test"
"$work/test" > "$work/log"
test "$(rg -c '^\[legendas\] addon avisou:' "$work/log")" = 2
if rg -q 'fixture\.invalid|PRIVATE_TOKEN|upstream failed|Internal Server Error' "$work/log"; then
  echo 'subtitle error diagnostic leaked upstream text' >&2
  exit 1
fi
rg '^PASS:' "$work/log"
