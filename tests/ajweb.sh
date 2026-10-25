#!/bin/bash
# Servidor de ajustes pelo navegador: rotas, codigo, Host/Origin e a caixa entre fios.
set -eu
cd "$(dirname "$0")/.."
T="${TMPDIR:-/tmp}/nuvio-ajweb"
cc -std=gnu11 -Wall -Wextra -Werror -Isrc tests/ajweb.c src/ajweb.c -lpthread -o "$T"
"$T" 8 > "$T.out" &
PID=$!
trap 'kill $PID 2>/dev/null || true' EXIT
for i in $(seq 1 50); do grep -q '^URL ' "$T.out" 2>/dev/null && break; sleep 0.1; done
URL=$(sed -n 's/^URL //p' "$T.out")
[ -n "$URL" ] || { echo "FALHOU: o servidor nao subiu (sem IP de rede local?)"; exit 1; }
PORTA=$(echo "$URL" | sed 's|.*:\([0-9]*\)/.*|\1|')
TOKEN=$(echo "$URL" | sed 's|.*/\([a-z0-9]*\)/$|\1|')
B="http://127.0.0.1:$PORTA"
ok() { echo "ok   $1"; }
falha() { echo "FALHOU: $1"; exit 1; }
[ "$(curl -s -o /dev/null -w '%{http_code}' $B/$TOKEN)" = 301 ] || falha "sem barra final deve redirecionar"
curl -s $B/$TOKEN/ | grep -q 'Nuvio' || falha "pagina"
ok "pagina"
[ "$(curl -s $B/$TOKEN/api/settings)" = '{"perfil":"Teste","s":[]}' ] || falha "esquema"
ok "esquema pela caixa"
R=$(curl -s -X POST --data-binary $'a 3\nb 4\n' -H 'Content-Type: text/plain' $B/$TOKEN/api/settings)
[ "$R" = '{"falhou":["b"],"ok":1}' ] || falha "aplicar: $R"
ok "aplicar"
[ "$(curl -s -o /dev/null -w '%{http_code}' $B/errado/api/settings)" = 404 ] || falha "codigo errado"
ok "codigo errado = 404"
[ "$(curl -s -o /dev/null -w '%{http_code}' -H 'Host: evil.example' $B/$TOKEN/)" = 403 ] || falha "Host de fora"
ok "Host de fora = 403"
[ "$(curl -s -o /dev/null -w '%{http_code}' -H 'Origin: http://evil.example' -X POST -d 'a 3' $B/$TOKEN/api/settings)" = 403 ] || falha "Origin de fora"
ok "Origin de fora = 403"
[ "$(curl -s -o /dev/null -w '%{http_code}' -X PUT $B/$TOKEN/api/settings)" = 405 ] || falha "metodo"
ok "metodo errado = 405"
wait $PID
echo "ajweb: ok"
