#!/bin/bash
# A conferencia real de uma fonte contra uma legenda: ver tests/fontesinc.c. MKV reais (ffmpeg + mkvmerge).
#   bash tests/fontesinc.sh    SANITIZE=1 (ASan/UBSan) ou SANITIZE=thread
set -euo pipefail
cd "$(dirname "$0")/.."
T=${TMPDIR:-/tmp}; FX="$T/nv-legref-fx"
bash tests/legref_fixtures.sh "$FX"
D=$(mktemp -d "$T/nv-fontesinc.XXXXXX"); trap 'rm -rf "$D"' EXIT
flags=()
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
elif [ "${SANITIZE:-0}" = thread ]; then flags+=(-fsanitize=thread -fno-omit-frame-pointer); fi
# FS_RITMO=0: sem o teto de Ranges por segundo (o teste le do disco).
cc "${flags[@]}" -DFS_RITMO=0 -Isrc -I/usr/include -I/usr/include/SDL2 -O1 -g -Wall -Wextra \
  src/fontesinc.c src/legref.c src/autosync.c src/legenda.c src/assrender.c tests/fontesinc.c -pthread -lm -o "$D/t"
"$D/t" "$FX"
