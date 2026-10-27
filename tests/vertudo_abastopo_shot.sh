#!/bin/bash
set -eu
cd "$(dirname "$0")/.."
mkdir -p ${TMPDIR:-/tmp}/rev/abastopo
NUVIO_SHOT_ABASTOPO=1 bash tests/vertudo_shot.sh ${TMPDIR:-/tmp}/rev/abastopo/colecao
