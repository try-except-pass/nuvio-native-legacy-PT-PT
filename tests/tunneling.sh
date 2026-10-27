#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-tunneling.XXXXXX")
trap 'rm -rf "$dir"' EXIT
for platform in desktop android; do
  flags=()
  if [ "$platform" = android ]; then flags+=(-DNV_ANDROID); fi
  cc -O1 -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -Isrc -I/usr/include -I/usr/include/SDL2 \
    -Wno-deprecated-declarations -Wno-macro-redefined \
    "${flags[@]}" tests/tunneling.c -o "$dir/$platform"
  "$dir/$platform"
done
python3 tests/tunneling_android.py
