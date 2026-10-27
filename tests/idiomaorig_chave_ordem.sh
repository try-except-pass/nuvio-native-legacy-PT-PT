#!/bin/bash
# A chave do TMDB tem de existir ANTES do app_iniciar: ele monta o catalogo do cache e pede o idioma original
# de cada titulo. Sem a chave o TMDB era pulado no lote todo (log 2.0.22: tmdb=- 0ms em todas as linhas).
set -euo pipefail
cd "$(dirname "$0")/.."
chave=$(grep -n '^ *desc_tmdb(dirArte);' src/main.c | head -1 | cut -d: -f1)
app=$(grep -n 'if (!app_iniciar(dirArte))' src/main.c | head -1 | cut -d: -f1)
[ -n "$chave" ] && [ -n "$app" ] || { echo "idiomaorig_chave_ordem: nao achei as chamadas em src/main.c"; exit 1; }
[ "$chave" -lt "$app" ] || { echo "idiomaorig_chave_ordem: FALHA, desc_tmdb (linha $chave) vem depois de app_iniciar (linha $app)"; exit 1; }
echo "idiomaorig_chave_ordem: ok (desc_tmdb linha $chave, app_iniciar linha $app)"
