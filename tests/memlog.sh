#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-memlog.XXXXXX")
trap 'rm -rf "$dir"' EXIT
for plataforma in linux mac; do
  flags=(-U__linux__)
  case "$plataforma" in
    linux) flags=(-D__linux__);;
  esac
  cc -std=c99 -Wall -Wextra -Werror "${flags[@]}" tests/memlog.c -o "$dir/test"
  "$dir/test"
done
