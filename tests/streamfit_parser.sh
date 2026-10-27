#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -euo pipefail
cd "$(dirname "$0")/.."
SF_DIR=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-streamfit-parser.XXXXXX)
trap 'rm -rf "$SF_DIR"' EXIT
flags=(-O1 -g -Wall -Wextra -Isrc -I/usr/include -ffunction-sections -fdata-sections)
if [ "$(uname -s)" = Darwin ]; then flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); else flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections); fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
cc "${flags[@]}" tests/streamfit_parser.c src/stream_parse.c src/fontefiltro.c src/linguas.c src/js.c src/badges.c -o "$SF_DIR/test"
"$SF_DIR/test"
