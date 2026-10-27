#!/bin/bash
# S5: execute the production opening renderer with recorded text/draw calls.
set -euo pipefail
cd "$(dirname "$0")/.."
export TMPDIR=${TMPDIR:-/tmp}
mkdir -p "$TMPDIR"
work=$(mktemp -d "$TMPDIR/nuvio-player-loading.XXXXXX")
trap 'rm -rf "$work"' EXIT
python3 - "$work/render.c" <<'PY'
from pathlib import Path
import sys
s = Path('src/player.c').read_text()
a = s.index('static void desenharCinema(')
b = s.index('\nvoid player_desenhar(', a)
Path(sys.argv[1]).write_text(s[a:b])
PY
cc tests/player_loading.c src/abrecine.c -I"$work" -Isrc \
  -I/usr/include -I/usr/include/SDL2 \
  -L/usr/lib/x86_64-linux-gnu -lSDL2 -lm -o "$work/test"
"$work/test"
