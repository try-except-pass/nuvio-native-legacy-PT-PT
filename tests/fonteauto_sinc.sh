#!/bin/bash
# A escolha que mira uma legenda sincronizada (HANDOFF §4 item 2): ver tests/fonteauto_sinc.c.
#
#   bash tests/fonteauto_sinc.sh
set -eu
cd "$(dirname "$0")/.."
cc -O2 -g -Wall -Wextra -Isrc src/fonteauto.c tests/fonteauto_sinc.c -o "${TMPDIR:-/tmp}/nuvio-fonteauto-sinc-tests"
"${TMPDIR:-/tmp}/nuvio-fonteauto-sinc-tests"
