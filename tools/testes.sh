#!/bin/bash
# A SUITE DE TODA RELEASE (dono, 2026-10-27: "toda release passa pelos mesmos testes, com resultado igual ou
# melhor"). Roda tests/*.sh em paralelo neste PC Linux e compara com a base gravada no repositorio.
#
#   bash tools/testes.sh                 # tudo; compara com tests/base.tsv
#   bash tools/testes.sh legsync fonte   # so os nomes que contem um destes trechos
#   bash tools/testes.sh --gravar-base   # tudo e grava tests/base.tsv (so depois de conferir o resultado)
#   JOBS=4 TEST_TIMEOUT=600 bash tools/testes.sh
#
# Saida: build/testes/ultimo.tsv (teste, estado, segundos) e build/testes/logs/<teste>.log.
# Estados: ok, falha, timeout, skip (saiu 0 dizendo SKIP: nao provou nada).
# REGRESSAO = estava ok na base e agora nao esta -> sai com 1. Os que melhoraram aparecem, nao falham.
# Fora da suite (nao servem de porteiro): *_shot (capturas), os de medida (perf, que vao em tools/bench.sh).
set -uo pipefail
cd "$(dirname "$0")/.." || exit 2
GRAVAR=0; FILTROS=()
for a in "$@"; do
  case "$a" in --gravar-base) GRAVAR=1;; *) FILTROS+=("$a");; esac
done
JOBS=${JOBS:-$(( $(nproc) > 2 ? $(nproc) - 1 : 1 ))}
export TEST_TIMEOUT=${TEST_TIMEOUT:-600}
OUT=build/testes; rm -rf "$OUT/logs"; mkdir -p "$OUT/logs"
BASE=tests/base.tsv
RUNTMP=$(mktemp -d "${TMPDIR:-/tmp}/nvtestes.XXXXXX"); export RUNTMP
trap 'rm -rf "$RUNTMP"' EXIT

lista() {
  for f in tests/*.sh; do
    n=$(basename "$f" .sh)
    case "$n" in *_shot|cinematic|director|*_perf|*_perf_*|salvospainel_perf) continue;; esac
    if [ ${#FILTROS[@]} -gt 0 ]; then
      ok=0; for p in "${FILTROS[@]}"; do case "$n" in *"$p"*) ok=1;; esac; done
      [ $ok = 1 ] || continue
    fi
    echo "$n"
  done
}
um() {
  n=$1; log="build/testes/logs/$n.log"; t0=$(date +%s.%N)
  TMPDIR=$(mktemp -d "$RUNTMP/$n.XXXXXX") timeout -k 5 "$TEST_TIMEOUT" bash "tests/$n.sh" >"$log" 2>&1
  rc=$?
  s=$(awk -v a="$t0" -v b="$(date +%s.%N)" 'BEGIN{printf "%.1f", b-a}')
  if [ $rc = 0 ]; then
    if grep -Eq '(^|[^A-Za-z])SKIP([^A-Za-z]|$)' "$log"; then e=skip; else e=ok; fi
  elif [ $rc = 124 ] || [ $rc = 137 ]; then e=timeout
  else e=falha; fi
  printf '%s\t%s\t%s\n' "$n" "$e" "$s"
}
export -f um
total=$(lista | wc -l)
echo "testes: $total, $JOBS em paralelo, teto ${TEST_TIMEOUT}s cada"
lista | xargs -P "$JOBS" -I{} bash -c 'um "$@"' _ {} | tee "$OUT/ultimo.tsv.tmp" | awk -F'\t' '$2!="ok"{print $2"\t"$1}'
sort "$OUT/ultimo.tsv.tmp" > "$OUT/ultimo.tsv"; rm -f "$OUT/ultimo.tsv.tmp"
awk -F'\t' '{c[$2]++} END{printf "resultado:"; for (k in c) printf " %s=%d", k, c[k]; print ""}' "$OUT/ultimo.tsv"

if [ $GRAVAR = 1 ]; then
  cut -f1,2 "$OUT/ultimo.tsv" > "$BASE"; echo "base gravada: $BASE"; exit 0
fi
[ -f "$BASE" ] || { echo "sem $BASE: rode com --gravar-base depois de conferir"; exit 1; }
# Compara so os testes desta rodada.
reg=$(join -t $'\t' <(cut -f1,2 "$BASE" | sort) <(cut -f1,2 "$OUT/ultimo.tsv") | awk -F'\t' '$2=="ok" && $3!="ok"{print $1" ("$3")"}')
mel=$(join -t $'\t' <(cut -f1,2 "$BASE" | sort) <(cut -f1,2 "$OUT/ultimo.tsv") | awk -F'\t' '$2!="ok" && $3=="ok"{print $1}')
novos=$(join -t $'\t' -v2 <(cut -f1,2 "$BASE" | sort) <(cut -f1,2 "$OUT/ultimo.tsv") | awk -F'\t' '{print $1" ("$2")"}')
[ -n "$mel" ] && { echo "melhoraram (atualize a base):"; echo "$mel" | sed 's/^/  /'; }
[ -n "$novos" ] && { echo "novos (fora da base):"; echo "$novos" | sed 's/^/  /'; }
if [ -n "$reg" ]; then echo "REGRESSOES:"; echo "$reg" | sed 's/^/  /'; exit 1; fi
echo "sem regressao contra $BASE"
