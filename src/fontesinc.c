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
  char refIdioma[24];         // idioma da faixa lida como referencia
  int refDialogo;             // 1 = faixa de dialogo (nem forcada nem so letreiros)
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
  l->refDoc = NULL; l->refFalhou = 0; l->url[0] = 0; l->refIdioma[0] = 0; l->refDialogo = 0;
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
  snprintf(l->refIdioma, sizeof l->refIdioma, "%s", st.idioma);
  l->refDialogo = !(legenda_documento_info(l->refDoc)->flags & (LEGENDA_DOC_FORCED | LEGENDA_DOC_SINAIS));
  return 1;
}

// A referencia de `url`: a guardada, ou le agora. -1 = cancelado; 0 = pronta (ou guardada como falha em l->refFalhou).
static int garantir(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                    int (*cancelado)(void *), void *u) {
  if (!strcmp(l->url, url) && l->sessao == sessao) return 0;
  return lerReferencia(l, url, sessao, idiomaRef, cancelado, u) < 0 ? -1 : 0;
}

int fontesinc_faixa(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                    int (*cancelado)(void *), void *u, char *idioma, unsigned tam, int *dialogo) {
  if (idioma && tam) idioma[0] = 0;
  if (dialogo) *dialogo = 0;
  if (!l || !url || !url[0]) return 0;
  if (garantir(l, url, sessao, idiomaRef, cancelado, u) < 0) return -1;
  if (l->refFalhou || !l->refDoc) return 0;
  if (idioma && tam) snprintf(idioma, tam, "%s", l->refIdioma);
  if (dialogo) *dialogo = l->refDialogo;
  return 1;
}

int fontesinc_julgar(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                     const LegendaDocumento *doc, int (*cancelado)(void *), void *u, int *offsetMs) {
  AutoSyncConfig cfg;
  AutoSyncResultado r;
  if (offsetMs) *offsetMs = 0;
  if (!l || !url || !url[0] || !doc) return FSINC_NAO;
  if (garantir(l, url, sessao, idiomaRef, cancelado, u) < 0) return FSINC_DESCONHECIDA;   // outra fonte: le a dela
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

int fontesinc_audio(FonteSincLeitor *l, const char *url, uint64_t sessao, int (*cancelado)(void *), void *u,
                    char *audio, unsigned tam) {
  LegRefOrcamento o = { FS_LEITURA_BYTES, FS_PEDIDOS, FS_RITMO, 1 };
  LegRefStatus st;
  uint64_t pedido;
  int ms = 0;
  if (audio && tam) audio[0] = 0;
  if (!l || !l->ref || !url || !url[0]) return 0;
  pedido = legref_pedir(l->ref, url, sessao, "", NULL, 0, &o);
  if (!pedido) return 0;
  for (;;) {
    st = legref_status(l->ref);
    if (st.fase != LEGREF_LENDO) break;
    if ((cancelado && cancelado(u)) || ms >= FS_PRAZO_MS) {
      legref_cancelar(l->ref);
      return ms >= FS_PRAZO_MS ? 0 : -1;
    }
    usleep(5000); ms += 5;
  }
  l->url[0] = 0;   // o pedido da cabeca substituiu o da referencia: a proxima garantir() le de novo
  if (st.motivo != LEGREF_SO_CABECA && st.motivo != LEGREF_OK) return 0;
  if (audio && tam) snprintf(audio, tam, "%s", st.audio);
  return 1;
}
