#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Websocket proprio contra o gateway real do Discord. PRECISA DE REDE.
# Ver o cabecalho de tests/discordws.c.
#
#   bash tests/discordws.sh
set -euo pipefail
cd "$(dirname "$0")/.."
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections -ffunction-sections -fdata-sections -Wl,--gc-sections \
       -I/usr/include -I/usr/include/SDL2 \
       -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" src/discordws.c src/rede.c src/js.c tests/discordws.c -o ${TMPDIR:-/tmp}/nuvio-discordws-tests
${TMPDIR:-/tmp}/nuvio-discordws-tests | tee /dev/stderr | grep -q 'discordws: ok'
echo "discordws.sh: ok"
