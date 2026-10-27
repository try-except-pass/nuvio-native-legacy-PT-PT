#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d /tmp/nuvio-pr536.XXXXXX)
trap 'rm -rf "$work"' EXIT
flags=(-Isrc -I/usr/include -I/usr/include/SDL2
  -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections -Wno-deprecated-declarations)
cc "${flags[@]}" -DTEST_SHEET tests/pr536_fixes.c -o "$work/sheet"
cc "${flags[@]}" -DNV_ASS_LIBASS tests/pr536_fixes.c \
  $(pkg-config --cflags --libs libass) -o "$work/shadow"
rc=0
"$work/sheet" || rc=1
"$work/shadow" || rc=1
exit "$rc"
