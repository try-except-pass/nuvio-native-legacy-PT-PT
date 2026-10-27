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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define DOCS_MAX 3          // legendas por idioma comparadas com cada fonte (a engine do legsync tenta ate 3 trocas)
#define ESPERA_LEGENDAS_MS 30000

typedef struct { unsigned tok; uint64_t sessao; int atual; } Ctx;
typedef struct { LegendaDocumento *d[DOCS_MAX]; char nome[DOCS_MAX][64]; int n; } Docs;
typedef struct { const Ctx *x; unsigned geracao; FonteSincLeitor *leitor; Docs docs[2]; const char *idiomaRef; } Cena;

static _Atomic unsigned vigente;          // o fio de um `tok` velho percebe e sai sozinho
static _Atomic int pedido = -1;           // a troca pedida (indice), -1 = nada
static _Atomic int pedidoDe = -1;         // a fonte que tocava quando a troca foi pedida
static _Atomic unsigned pedidoTok;        // de que conferencia veio
static _Atomic unsigned jaTrocou;         // `tok` que ja trocou: no maximo UMA troca por reproducao

static int vivo(const Ctx *x) { return atomic_load(&vigente) == x->tok && nv_perfil_sessao_atual(x->sessao); }
static int cancelado(void *u) { return !vivo(u); }

static void soltarDocs(Docs *d) {
  for (int i = 0; i < d->n; i++) legenda_documento_liberar(d->d[i]);
  d->n = 0;
}

// Baixa ate DOCS_MAX legendas do idioma `pref` da lista, como documentos AVULSOS: nada passa pelo overlay nem pela
// legenda que a pessoa esta vendo (legenda_carregar_com liga no overlay; aqui so rede_baixar_bin + um documento).
static void baixarDocs(const Ctx *x, const char *pref, const Legenda *v, int nv, Docs *out) {
  for (int i = 0; i < nv && out->n < DOCS_MAX && vivo(x); i++) {
    LegendaDocumentoInfo info;
    LegendaDocumento *d;
    long n = 0;
    char *corpo;
    if (!ling_casa(v[i].idioma, pref)) continue;
    corpo = rede_baixar_bin(v[i].url, 15, &n);
    if (!corpo || n < 1) { free(corpo); continue; }
    memset(&info, 0, sizeof info);
    info.sessao = x->sessao; info.flags = LEGENDA_DOC_COMPLETO;
    snprintf(info.idioma, sizeof info.idioma, "%s", pref);
    snprintf(info.origem, sizeof info.origem, "%s", v[i].provedor);
    snprintf(info.identidade, sizeof info.identidade, "%s", v[i].id[0] ? v[i].id : v[i].url);
    d = legenda_documento_bytes(corpo, n, &info);
    free(corpo);
    if (!d) continue;   // corpo que nao e legenda (pagina de erro, arquivo quebrado): fora
    out->d[out->n] = d;
    snprintf(out->nome[out->n], sizeof out->nome[0], "%s", v[i].arquivo[0] ? v[i].arquivo : v[i].rotulo);
    out->n++;
  }
}

// O gancho da conferencia em serie: a legenda do `slot` contra a fonte `i`, tentando as DOCS_MAX legendas do idioma.
static int conferir(int i, int slot, void *u) {
  Cena *c = u;
  char url[4096];
  int off = 0, veredito = FSINC_NAO;
  if (!stream_sinc_link(c->geracao, i, url, sizeof url)) {
    printf("[sinc-fonte] fonte %d: sem link direto pronto; nao conferida\n", i);
    return FSINC_DESCONHECIDA;
  }
  for (int k = 0; k < c->docs[slot].n && vivo(c->x); k++) {
    int r = fontesinc_julgar(c->leitor, url, c->x->sessao, c->idiomaRef, c->docs[slot].d[k], cancelado, (void *)c->x, &off);
    printf("[sinc-fonte] fonte %d legenda %s #%d \"%s\": %s (%s) %+d ms\n", i, slot ? "2" : "1", k, c->docs[slot].nome[k],
           r == FSINC_CONFIRMADA ? "sincroniza" : r == FSINC_NAO ? "nao" : "cancelada", fontesinc_motivo(c->leitor), off);
    if (r == FSINC_CONFIRMADA) { veredito = r; break; }
    if (r == FSINC_DESCONHECIDA) { veredito = r; break; }
  }
  fflush(stdout);
  return veredito;
}

// O nivel da fonte que toca: 1 = principal confirmada, 2 = secundaria confirmada, 99 = sem confirmacao.
static int nivelDe(const FonteSinc *s) {
  return s->sincPrim == FSINC_CONFIRMADA ? FSINC_PRIM_OK : s->sincSec == FSINC_CONFIRMADA ? FSINC_SEC_OK : 99;
}

static void *fio(void *arg) {
  Ctx *x = arg;
  int fila[16], lembrada = -1, n, total, exigeOriginal, temSec, nivel = 0, feitas = 0, melhor, esperado = 0;
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
  baixarDocs(x, prim, leg, nleg, &cena.docs[0]);
  if (temSec) baixarDocs(x, sec, leg, nleg, &cena.docs[1]);
  printf("[sinc-fonte] %d fonte(s) na fila, %d legenda(s) %s%s%s\n", n, cena.docs[0].n, prim,
         temSec ? " + secundaria " : "", temSec ? sec : "");
  fflush(stdout);
  if (!cena.docs[0].n && !(temSec && cena.docs[1].n)) { printf("[sinc-fonte] nenhuma legenda para comparar\n"); goto fim; }
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
  g.conferir = conferir; g.cancelado = cancelado; g.u = &cena;
  melhor = fonteauto_sincronizar(fila, n, c, exigeOriginal, temSec, &g, &nivel, &feitas);
  printf("[sinc-fonte] %d fonte(s) conferida(s): melhor=%d nivel=%d; tocando=%d (nivel %d)\n", feitas, melhor, nivel, x->atual,
         x->atual >= 0 && x->atual < total ? nivelDe(&c[x->atual]) : 99);
  fflush(stdout);
  // so uma sincronia CONFIRMADA justifica trocar, e so se a que toca nao tem uma tao boa
  if (vivo(x) && melhor >= 0 && melhor != x->atual && (nivel == FSINC_PRIM_OK || nivel == FSINC_SEC_OK) &&
      (x->atual < 0 || x->atual >= total || nivelDe(&c[x->atual]) > nivel) && atomic_load(&jaTrocou) != x->tok) {
    atomic_store(&pedidoDe, x->atual);
    atomic_store(&pedidoTok, x->tok);
    atomic_store(&pedido, melhor);
    printf("[sinc-fonte] pede a troca da fonte %d para a %d (legenda sincronizada)\n", x->atual, melhor);
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
  x->sessao = sessao; x->atual = atual;
  atomic_store(&pedido, -1);
  if (pthread_create(&t, NULL, fio, x) == 0) pthread_detach(t);
  else free(x);
}

void fontesincapp_cancelar(void) {
  atomic_fetch_add(&vigente, 1);
  atomic_store(&pedido, -1);
}

int fontesincapp_trocar(int *indice, int *atual) {
  int p = atomic_load(&pedido);
  if (p < 0 || atomic_load(&pedidoTok) != atomic_load(&vigente)) return 0;
  if (!atomic_compare_exchange_strong(&pedido, &p, -1)) return 0;
  atomic_store(&jaTrocou, atomic_load(&pedidoTok));   // esta reproducao nao troca de novo
  if (indice) *indice = p;
  if (atual) *atual = atomic_load(&pedidoDe);
  return 1;
}
