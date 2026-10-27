#include "fontesincapp.h"
#include "addons.h"
#include "fonteauto.h"
#include "fontesinc.h"
#include "legenda.h"
#include "linguas.h"
#include "perfisessao.h"
#include "rede.h"
#include "streams.h"
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define ESPERA_LEGENDAS_MS 30000
// Teto de TEMPO da conferencia inteira, nao de legendas nem de fontes: comparar custa ~2 ms no PC, o que custa e a rede
// (baixar uma legenda, ler cada fonte). Estourado, vale o que ja se sabe.
#define PRAZO_TOTAL_MS 180000

typedef struct { unsigned tok; uint64_t sessao; int atual; long long ate; } Ctx;
// As legendas de UM idioma: `idx` = as da lista que casam com o idioma (TODAS); `d` so existe depois de baixada, e so
// e baixada quando a conferencia chega nela (a 1a que sincroniza encerra a procura, as demais nem sao baixadas).
typedef struct {
  const Legenda *v; int idx[LEG_MAX]; int n;
  LegendaDocumento *d[LEG_MAX]; char baixou[LEG_MAX];
  char pref[24];
} Docs;
typedef struct { const Ctx *x; unsigned geracao; FonteSincLeitor *leitor; Docs docs[2]; const char *idiomaRef; } Cena;

static _Atomic unsigned vigente;          // o fio de um `tok` velho percebe e sai sozinho
static _Atomic int pedido = -1;           // a troca pedida (indice), -1 = nada
static _Atomic int pedidoDe = -1;         // a fonte que tocava quando a troca foi pedida
static _Atomic unsigned pedidoTok;        // de que conferencia veio
static _Atomic int pedidoSinc;            // 1 = a legenda da fonte pedida esta com a sincronia confirmada
static _Atomic unsigned jaTrocou;         // `tok` que ja trocou: no maximo UMA troca por reproducao

static long long agoraMs(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
static int vivo(const Ctx *x) { return atomic_load(&vigente) == x->tok && nv_perfil_sessao_atual(x->sessao); }
static int cancelado(void *u) { const Ctx *x = u; return !vivo(x) || agoraMs() > x->ate; }

static void soltarDocs(Docs *d) {
  for (int i = 0; i < d->n; i++) legenda_documento_liberar(d->d[i]);
  d->n = 0;
}

// Quais legendas da lista casam com o idioma `pref`: TODAS, na ordem da lista. Nada e baixado aqui.
static void escolherDocs(const char *pref, const Legenda *v, int nv, Docs *out) {
  out->v = v;
  snprintf(out->pref, sizeof out->pref, "%s", pref);
  for (int i = 0; i < nv && out->n < LEG_MAX; i++)
    if (ling_casa(v[i].idioma, pref)) out->idx[out->n++] = i;
}

// Baixa a legenda k (uma vez) como documento AVULSO: nada passa pelo overlay nem pela legenda que a pessoa esta vendo
// (legenda_carregar_com liga no overlay; aqui so rede_baixar_bin + um documento). NULL = nao veio ou nao e legenda.
static LegendaDocumento *baixarUma(const Ctx *x, Docs *dd, int k) {
  const Legenda *l = &dd->v[dd->idx[k]];
  LegendaDocumentoInfo info;
  long n = 0;
  char *corpo;
  if (dd->baixou[k]) return dd->d[k];
  dd->baixou[k] = 1;
  if (!vivo(x)) return NULL;
  corpo = rede_baixar_bin(l->url, 15, &n);
  if (!corpo || n < 1) { free(corpo); return NULL; }
  memset(&info, 0, sizeof info);
  info.sessao = x->sessao; info.flags = LEGENDA_DOC_COMPLETO;
  snprintf(info.idioma, sizeof info.idioma, "%s", dd->pref);
  snprintf(info.origem, sizeof info.origem, "%s", l->provedor);
  snprintf(info.identidade, sizeof info.identidade, "%s", l->id[0] ? l->id : l->url);
  dd->d[k] = legenda_documento_bytes(corpo, n, &info);   // corpo que nao e legenda (pagina de erro): NULL
  free(corpo);
  return dd->d[k];
}

// O gancho da conferencia em serie: a legenda do `slot` contra a fonte `i`, tentando TODAS as legendas do idioma, baixadas so quando chega nelas.
static int conferir(int i, int slot, void *u) {
  Cena *c = u;
  char url[4096];
  int off = 0, veredito = FSINC_NAO;
  if (!stream_sinc_link(c->geracao, i, url, sizeof url)) {
    printf("[sinc-fonte] fonte %d: sem link direto pronto; nao conferida\n", i);
    return FSINC_DESCONHECIDA;
  }
  for (int k = 0; k < c->docs[slot].n && !cancelado((void *)c->x); k++) {
    const Legenda *l = &c->docs[slot].v[c->docs[slot].idx[k]];
    LegendaDocumento *d = baixarUma(c->x, &c->docs[slot], k);
    int r;
    if (!d) continue;
    r = fontesinc_julgar(c->leitor, url, c->x->sessao, c->idiomaRef, d, cancelado, (void *)c->x, &off);
    printf("[sinc-fonte] fonte %d legenda %s #%d/%d \"%s\": %s (%s) %+d ms\n", i, slot ? "2" : "1", k + 1, c->docs[slot].n,
           l->arquivo[0] ? l->arquivo : l->rotulo,
           r == FSINC_CONFIRMADA ? "sincroniza" : r == FSINC_NAO ? "nao" : "cancelada", fontesinc_motivo(c->leitor), off);
    if (r == FSINC_CONFIRMADA) { veredito = r; break; }
    if (r == FSINC_DESCONHECIDA) { veredito = r; break; }
  }
  fflush(stdout);
  return veredito;
}

// A faixa EMBUTIDA no idioma principal: a mesma referencia que a conferencia le (lida uma vez por fonte). Uma faixa de
// dialogo no idioma da pessoa dentro da fonte foi multiplexada com o video: sincronizada de fabrica, degrau 1, sem
// precisar de legenda de addon. Faixa forcada ou so de letreiros nao conta.
static int embutida(int i, void *u) {
  Cena *c = u;
  char url[4096], idioma[24];
  int dialogo = 0;
  if (!stream_sinc_link(c->geracao, i, url, sizeof url)) return 0;
  if (fontesinc_faixa(c->leitor, url, c->x->sessao, c->idiomaRef, cancelado, (void *)c->x, idioma, sizeof idioma, &dialogo) != 1)
    return 0;
  if (!dialogo || !idioma[0] || !ling_casa(idioma, c->idiomaRef)) return 0;
  printf("[sinc-fonte] fonte %d: traz uma faixa de dialogo em %s dentro dela (sincronizada de fabrica)\n", i, idioma);
  fflush(stdout);
  return 1;
}

// ONDE UMA FONTE ESTA NA LISTA DO DONO, menor = melhor: o audio original separa tudo (+10 para quem nao e original
// quando alguma e); depois 1 principal sincronizada, 2 secundaria sincronizada, 3 principal sem confirmacao,
// 4 secundaria sem confirmacao, 5 sem legenda. Troca-se quando a MELHOR da fila esta num lugar estritamente melhor que
// o da que toca: qualquer degrau da lista justifica, nao so a sincronia confirmada.
static int lugar(const FonteSinc *s, int soOriginal, int temSec) {
  int r = soOriginal && !s->original ? 10 : 0;
  return r + (s->sincPrim == FSINC_CONFIRMADA ? 1
            : temSec && s->sincSec == FSINC_CONFIRMADA ? 2
            : s->temPrim ? 3
            : temSec && s->temSec ? 4 : 5);
}

static void *fio(void *arg) {
  Ctx *x = arg;
  int fila[16], lembrada = -1, n, total, exigeOriginal, temSec, nivel = 0, feitas = 0, melhor, esperado = 0;
  int soOriginal, atualNaFila = 0;
  unsigned char original[16];
  unsigned geracao = 0;
  char primB[24], secB[24];
  const char *prim = primB, *sec = secB;
  Legenda *leg = NULL;
  int nleg = 0, prontas = 0;
  FonteSinc *c = NULL;
  Cena cena;
  FonteSincGancho g;

  memset(&cena, 0, sizeof cena);
  // copias: ling_*() devolve ponteiros que a UI pode trocar no meio de uma conferencia longa
  snprintf(primB, sizeof primB, "%s", ling_legenda());
  snprintf(secB, sizeof secB, "%s", ling_legenda2());
  n = stream_sinc_fila(x->sessao, &geracao, &lembrada, fila, original, 16);   // 16 = VER_MAX de streams.c
  x->ate = agoraMs() + PRAZO_TOTAL_MS;
  if (n < 2) { printf("[sinc-fonte] %d fonte(s) na fila; nada a comparar\n", n); goto fim; }
  if (!prim || !prim[0] || !strcasecmp(prim, "none")) { printf("[sinc-fonte] sem idioma de legenda; nada a conferir\n"); goto fim; }
  if (x->atual == lembrada) { printf("[sinc-fonte] fonte lembrada do titulo (%d); nao troca\n", x->atual); goto fim; }

  // a lista de legendas do titulo (a busca dos addons termina em segundos)
  leg = malloc(sizeof *leg * LEG_MAX);
  if (!leg) goto fim;
  while (vivo(x)) {
    nleg = addons_legendas_copiar(leg, LEG_MAX, NULL, &prontas);
    if (prontas || esperado >= ESPERA_LEGENDAS_MS) break;
    usleep(250000); esperado += 250;
  }
  if (!vivo(x)) goto fim;
  temSec = sec && sec[0] && strcasecmp(sec, "none") && !(ling_casa(sec, prim) && ling_casa(prim, sec));
  cena.x = x; cena.geracao = geracao; cena.idiomaRef = prim;
  escolherDocs(prim, leg, nleg, &cena.docs[0]);
  if (temSec) escolherDocs(sec, leg, nleg, &cena.docs[1]);
  printf("[sinc-fonte] %d fonte(s) na fila, %d legenda(s) %s%s%s\n", n, cena.docs[0].n, prim,
         temSec ? " + secundaria " : "", temSec ? sec : "");
  fflush(stdout);
  if (!cena.docs[0].n && !(temSec && cena.docs[1].n))
    printf("[sinc-fonte] nenhuma legenda de addon; so a faixa embutida nas fontes pode servir\n");
  temSec = temSec && cena.docs[1].n > 0;

  total = stream_n();
  // a fila e um retrato: lista trocada no meio (outro titulo, busca terminada) e os indices nao valem mais
  if (stream_lista_geracao() != geracao) { printf("[sinc-fonte] a lista de fontes mudou; descartado\n"); goto fim; }
  for (int q = 0; q < n; q++)
    if (fila[q] < 0 || fila[q] >= total) { printf("[sinc-fonte] indice fora da lista; descartado\n"); goto fim; }
  c = calloc((size_t)(total > 0 ? total : 1), sizeof *c);
  if (!c) goto fim;
  for (int q = 0; q < n; q++) {
    if (fila[q] < 0 || fila[q] >= total) continue;
    c[fila[q]].original = original[q];
    c[fila[q]].temPrim = (unsigned char)(cena.docs[0].n > 0);
    c[fila[q]].temSec = (unsigned char)(temSec);
  }
  cena.leitor = fontesinc_leitor_criar(NULL, NULL);
  if (!cena.leitor) goto fim;
  exigeOriginal = ling_audio_e_original();
  g.conferir = conferir; g.cancelado = cancelado; g.u = &cena; g.embutida = embutida;
  melhor = fonteauto_sincronizar(fila, n, c, exigeOriginal, temSec, &g, &nivel, &feitas);
  soOriginal = 0;
  if (exigeOriginal) for (int q = 0; q < n; q++) soOriginal |= c[fila[q]].original;
  for (int q = 0; q < n; q++) if (fila[q] == x->atual) atualNaFila = 1;
  if (melhor >= 0 && atualNaFila)
    printf("[sinc-fonte] %d fonte(s) conferida(s): melhor=%d (lugar %d); tocando=%d (lugar %d)\n", feitas, melhor,
           lugar(&c[melhor], soOriginal, temSec), x->atual, lugar(&c[x->atual], soOriginal, temSec));
  fflush(stdout);
  // troca quando a melhor da fila esta num lugar ESTRITAMENTE melhor da lista do que a que toca (a que toca tem de
  // estar na fila: fora dela nao ha com o que comparar)
  if (vivo(x) && melhor >= 0 && melhor != x->atual && atualNaFila &&
      lugar(&c[melhor], soOriginal, temSec) < lugar(&c[x->atual], soOriginal, temSec) &&
      atomic_load(&jaTrocou) != x->tok) {
    atomic_store(&pedidoDe, x->atual);
    atomic_store(&pedidoTok, x->tok);
    atomic_store(&pedidoSinc, c[melhor].sincPrim == FSINC_CONFIRMADA || (temSec && c[melhor].sincSec == FSINC_CONFIRMADA));
    atomic_store(&pedido, melhor);
    printf("[sinc-fonte] pede a troca da fonte %d (lugar %d) para a %d (lugar %d)\n", x->atual,
           lugar(&c[x->atual], soOriginal, temSec), melhor, lugar(&c[melhor], soOriginal, temSec));
    fflush(stdout);
  }
fim:
  if (cena.leitor) fontesinc_leitor_destruir(cena.leitor);
  soltarDocs(&cena.docs[0]); soltarDocs(&cena.docs[1]);
  free(c); free(leg); free(x);
  return NULL;
}

void fontesincapp_iniciar(uint64_t sessao, int atual) {
  pthread_t t;
  Ctx *x = malloc(sizeof *x);
  if (!x) return;
  x->tok = atomic_fetch_add(&vigente, 1) + 1;   // o fio anterior, se corre, percebe e sai
  x->sessao = sessao; x->atual = atual; x->ate = 0;
  atomic_store(&pedido, -1);
  if (pthread_create(&t, NULL, fio, x) == 0) pthread_detach(t);
  else free(x);
}

void fontesincapp_cancelar(void) {
  atomic_fetch_add(&vigente, 1);
  atomic_store(&pedido, -1);
}

int fontesincapp_trocar(int *indice, int *atual, int *sincronizada) {
  int p = atomic_load(&pedido);
  if (p < 0 || atomic_load(&pedidoTok) != atomic_load(&vigente)) return 0;
  if (!atomic_compare_exchange_strong(&pedido, &p, -1)) return 0;
  atomic_store(&jaTrocou, atomic_load(&pedidoTok));   // esta reproducao nao troca de novo
  if (indice) *indice = p;
  if (atual) *atual = atomic_load(&pedidoDe);
  if (sincronizada) *sincronizada = atomic_load(&pedidoSinc);
  return 1;
}
