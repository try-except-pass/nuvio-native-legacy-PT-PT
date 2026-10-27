#!/bin/bash
# play[]/nPlay do Trakt: trakt_playback_remover x trakt_continuar em fios
# diferentes, sob ThreadSanitizer. Relato de corrida = falha.
#   bash tests/trakt_play_corrida.sh
set -euo pipefail
cd "$(dirname "$0")/.."
export TMPDIR="${TMPDIR:-/tmp}"
dir=$(mktemp -d "$TMPDIR/nuvio-traktplay.XXXXXX")
trap 'rm -rf "$dir"' EXIT
cc src/trakt.c src/metaep.c tests/stub_fichameta.c src/js.c src/metaprov.c tests/trakt_play_corrida.c \
  -Isrc -I/usr/include -I/usr/include/SDL2 \
  -o "$dir/teste" -O1 -g -fsanitize=thread -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections -lpthread
rc=0
TSAN_OPTIONS="halt_on_error=1 exitcode=66" "$dir/teste" >"$dir/saida" 2>"$dir/erro" || rc=$?
if [ "$rc" != 0 ] || grep -q "ThreadSanitizer" "$dir/erro"; then
  grep -A14 "WARNING: ThreadSanitizer" "$dir/erro" | head -30 || true
  echo "FALHOU: corrida em play[]/nPlay (rc=$rc)"
  exit 1
fi
tail -1 "$dir/saida"
