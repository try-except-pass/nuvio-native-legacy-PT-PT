#!/usr/bin/env bash
set -euo pipefail
node ${TMPDIR:-/tmp}/musica-visual/render.cjs "${@:---check-only}"
