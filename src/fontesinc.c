#include "fontesinc.h"
#include "autosync.h"
#include "fonteauto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Mesmo orcamento do legsync (uma faixa); o ritmo baixo de Ranges por segundo existe para nao disputar a conexao do
// video que ja esta tocando. O teste le do disco e zera o ritmo (-DFS_RITMO=0), como o LS_RITMO do legsync.
#define FS_LEITURA_BYTES (12LL * 1024 * 1024)
#define FS_PEDIDOS 6000
#ifndef FS_RITMO
#define FS_RITMO 8
#endif
#define FS_PRAZO_MS 45000   // teto de uma leitura: a legref tem o proprio orcamento, mas rede travada nao pode prender o laco

struct FonteSincLeitor {
  LegRef *ref;
  char url[4096];
  uint64_t sessao;
  LegendaDocumento *refDoc;   // a referencia da ultima url (guardada para o proximo slot)
  int refFalhou;              // a leitura da ultima url nao deu referencia: nao tenta de novo
  char motivo[40];
};

FonteSincLeitor *fontesinc_leitor_criar(LegRefLer ler, void *u) {
  FonteSincLeitor *l = calloc(1, sizeof *l);
  if (!l) return NULL;
  l->ref = (ler || legref_disponivel()) ? legref_criar(ler, u) : NULL;
  snprintf(l->motivo, sizeof l->motivo, "platform");
  return l;
}

static void soltar(FonteSincLeitor *l) {
  legenda_documento_liberar(l->refDoc);
  l->refDoc = NULL; l->refFalhou = 0; l->url[0] = 0;
}

void fontesinc_leitor_destruir(FonteSincLeitor *l) {
  if (!l) return;
  if (l->ref) legref_destruir(l->ref);
  soltar(l);
  free(l);
}

const char *fontesinc_motivo(const FonteSincLeitor *l) { return l ? l->motivo : "none"; }

// Le a referencia de `url` (uma vez). 0 = nao veio (l->motivo diz por que), -1 = cancelado.
static int lerReferencia(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                         int (*cancelado)(void *), void *u) {
  LegRefOrcamento o = { FS_LEITURA_BYTES, FS_PEDIDOS, FS_RITMO };
  LegRefStatus st;
  uint64_t pedido;
  int ms = 0;
  soltar(l);
  snprintf(l->url, sizeof l->url, "%s", url);
  l->sessao = sessao;
  if (!l->ref) { snprintf(l->motivo, sizeof l->motivo, "platform"); l->refFalhou = 1; return 0; }
  pedido = legref_pedir(l->ref, url, sessao, idiomaRef, NULL, 0, &o);
  if (!pedido) { snprintf(l->motivo, sizeof l->motivo, "no_track"); l->refFalhou = 1; return 0; }
  for (;;) {
    st = legref_status(l->ref);
    if (st.fase != LEGREF_LENDO) break;
    if ((cancelado && cancelado(u)) || ms >= FS_PRAZO_MS) {
      legref_cancelar(l->ref);
      if (ms >= FS_PRAZO_MS) { snprintf(l->motivo, sizeof l->motivo, "timeout"); l->refFalhou = 1; return 0; }
      return -1;
    }
    usleep(5000); ms += 5;
  }
  if (st.fase == LEGREF_PRONTO) l->refDoc = legref_tomar(l->ref, pedido);
  if (!l->refDoc) {
    snprintf(l->motivo, sizeof l->motivo, "%s", legref_motivo(st.motivo));
    l->refFalhou = 1;
    return 0;
  }
  return 1;
}

int fontesinc_julgar(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                     const LegendaDocumento *doc, int (*cancelado)(void *), void *u, int *offsetMs) {
  AutoSyncConfig cfg;
  AutoSyncResultado r;
  if (offsetMs) *offsetMs = 0;
  if (!l || !url || !url[0] || !doc) return FSINC_NAO;
  if (strcmp(l->url, url) || l->sessao != sessao) {   // outra fonte: le a referencia dela
    int e = lerReferencia(l, url, sessao, idiomaRef, cancelado, u);
    if (e < 0) return FSINC_DESCONHECIDA;
  }
  if (l->refFalhou || !l->refDoc) return FSINC_NAO;     // a mesma fonte ja falhou: o mesmo veredito, sem ler de novo
  cfg = autosync_config(AUTOSYNC_QUICK);
  r = autosync_comparar(doc, l->refDoc, &cfg, cancelado, u);
  if (r.estado == AUTOSYNC_CANCELLED) return FSINC_DESCONHECIDA;
  snprintf(l->motivo, sizeof l->motivo, "%s", autosync_motivo(r.motivo));
  if (r.estado == AUTOSYNC_ACCEPTED && !r.referenciaInvalida) {
    if (offsetMs) *offsetMs = r.offsetMs;
    return FSINC_CONFIRMADA;
  }
  return FSINC_NAO;
}
