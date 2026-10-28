#include "fontesincapp.h"
#include "addons.h"
#include "fonteauto.h"
#include "fontesinc.h"
#include "legenda.h"
#include "linguas.h"
#include "perfisessao.h"
#include "naovideo.h"
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
#define PRAZO_TOTAL_MS 600000   // a lista inteira, uma fonte por vez: 180 s nao chegava para 70 fontes

#define SINC_MAX 128   // = VER_MAX de streams.c: a lista inteira depois dos filtros
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
static _Atomic unsigned semLegendaTok;   // `tok` da conferencia que nao achou legenda em fonte nenhuma
static _Atomic unsigned jaTrocou;         // `tok` que ja trocou: no maximo UMA troca por reproducao

static long long agoraMs(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
static int vivo(const Ctx *x) { return atomic_load(&vigente) == x->tok && nv_perfil_sessao_atual(x->sessao); }
static int cancelado(void *u) { const Ctx *x = u; return !vivo(x) || agoraMs() > x->ate; }

// o gancho recebe a Cena (g.u), nao o Ctx: desembrulha antes de perguntar
static int canceladoCena(void *u) { return cancelado((void *)((Cena *)u)->x); }

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
  { int rf = fontesinc_faixa(c->leitor, url, c->x->sessao, c->idiomaRef, cancelado, (void *)c->x, NULL, 0, NULL);
    if (rf < 0) return FSINC_DESCONHECIDA;
    if (rf == 0) {   // nada para comparar: nao baixa legenda nenhuma
      printf("[sinc-fonte] fonte %d legenda %s: sem referencia (%s); nenhuma legenda baixada\n", i, slot ? "2" : "1", fontesinc_motivo(c->leitor));
      fflush(stdout);
      return FSINC_NAO;
    } }
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

// Ganchos da nota "provavelmente sincronizada": a fonte sem referencia, a duracao do arquivo, e o nome do arquivo.
static int refAusente(int i, long *duracaoMs, void *u) {
  Cena *c = u;
  (void)i;
  if (!fontesinc_referencia_ausente(c->leitor)) return 0;
  if (duracaoMs) *duracaoMs = fontesinc_duracao_ms(c->leitor);
  return 1;
}
static long duracaoFonte(int i, void *u) { Cena *c = u; (void)i; return fontesinc_duracao_ms(c->leitor); }
// "TheGodfather.1972.mkv" e "thegodfather 1972.srt" (so letras e digitos, sem a extensao): o mesmo arquivo.
static void chaveNome(const char *in, char *out, size_t tam) {
  size_t k = 0;
  const char *ponto = strrchr(in, '.');
  size_t n = ponto && strlen(ponto) <= 5 ? (size_t)(ponto - in) : strlen(in);
  for (size_t j = 0; j < n && k + 1 < tam; j++)
    if ((in[j] >= '0' && in[j] <= '9') || ((in[j] | 32) >= 'a' && (in[j] | 32) <= 'z')) out[k++] = (char)(in[j] | 32);
  out[k] = 0;
}
static int nomeCasa(int i, int slot, void *u) {
  Cena *c = u;
  char arq[512], ka[512], kl[512];
  if (!stream_sinc_arquivo(c->geracao, i, arq, sizeof arq)) return 0;
  chaveNome(arq, ka, sizeof ka);
  if (strlen(ka) < 12) return 0;   // nome curto demais para provar qualquer coisa
  for (int k = 0; k < c->docs[slot].n; k++) {
    const Legenda *l = &c->docs[slot].v[c->docs[slot].idx[k]];
    chaveNome(l->arquivo[0] ? l->arquivo : l->rotulo, kl, sizeof kl);
    if (!strcmp(ka, kl)) return 1;
  }
  return 0;
}

// O ARQUIVO da fonte tem o audio original? A etiqueta do addon quase nunca marca o idioma de um filme em ingles, entao
// a fonte "sem etiqueta" e conferida pelas faixas do proprio arquivo (a mesma leitura da cabeca da referencia).
// 1 = tem uma faixa de audio no idioma, 0 = leu as faixas e nao tem, -1 = nao deu para saber (MP4, sem link, rede).
static int audioOriginal(int i, void *u) {
  Cena *c = u;
  char url[4096], audio[64], *p, *fim;
  const char *pref = ling_audio();
  if (!pref || !pref[0] || !stream_sinc_link(c->geracao, i, url, sizeof url)) return -1;
  if (fontesinc_audio(c->leitor, url, c->x->sessao, cancelado, (void *)c->x, audio, sizeof audio) != 1 || !audio[0])
    return -1;
  for (p = audio; *p; p = fim ? fim + 1 : p + strlen(p)) {
    char cod[24];
    size_t n;
    fim = strchr(p, ',');
    n = fim ? (size_t)(fim - p) : strlen(p);
    if (n >= sizeof cod) n = sizeof cod - 1;
    memcpy(cod, p, n); cod[n] = 0;
    if (ling_casa(cod, pref)) {
      printf("[sinc-fonte] fonte %d: o arquivo traz audio em %s (a etiqueta nao dizia): conta como original\n", i, pref);
      fflush(stdout);
      return 1;
    }
    if (!fim) break;
  }
  printf("[sinc-fonte] fonte %d: audio do arquivo = %s; sem %s: fora\n", i, audio, pref);
  fflush(stdout);
  return 0;
}

// A fonte esta VIVA? Uma legenda sincronizada sem video nao serve: a mesma sonda da escolha (resposta de video, nao de erro).
static int fonteViva(const Cena *c, int i) {
  char url[4096], fim[4096], mime[96] = "";
  int http = 0;
  long corpo = -1;
  if (!stream_sinc_link(c->geracao, i, url, sizeof url)) return 0;
  if (!rede_url_final_tipo(url, 8, NULL, fim, sizeof fim, &http, mime, sizeof mime, &corpo)) return 0;
  if (http < 200 || http >= 400) return 0;
  return !naovideo_mime(mime, url, corpo);
}

static void *fio(void *arg) {
  Ctx *x = arg;
  int fila[SINC_MAX], lembrada = -1, n, total, exigeOriginal, temSec, nivel = 0, feitas = 0, melhor, esperado = 0;
  int atualNaFila = 0, nivelAtual;
  long *pontos = NULL;
  unsigned char original[SINC_MAX];
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
  n = stream_sinc_fila(x->sessao, &geracao, &lembrada, fila, original, SINC_MAX);
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
  g.conferir = conferir; g.cancelado = canceladoCena; g.u = &cena; g.embutida = embutida; g.audioOriginal = audioOriginal;
  g.refAusente = refAusente; g.duracao = duracaoFonte; g.nomeCasa = nomeCasa;
  // a que TOCA tambem: sem etiqueta de audio ela seria contada como "nao original" so por falta de marca
  if (exigeOriginal)
    for (int q = 0; q < n; q++)
      if (fila[q] == x->atual && !c[x->atual].original) {
        int r = audioOriginal(x->atual, &cena);
        if (r == 1) c[x->atual].original = 1; else if (r == 0) c[x->atual].outro = 1;
      }
  melhor = fonteauto_sincronizar(fila, n, c, exigeOriginal, temSec, &g, &nivel, &feitas);
  pontos = calloc((size_t)(total > 0 ? total : 1), sizeof *pontos);
  if (pontos) melhor = fonteauto_classificar(fila, n, c, temSec, pontos, &nivel);   // idempotente: so devolve tambem os pontos
  for (int q = 0; q < n; q++) if (fila[q] == x->atual) atualNaFila = 1;
  // o nivel de quem toca: fora da lista de quem tem audio original (arquivo lido, sem ele) vale pior que qualquer nivel
  nivelAtual = !atualNaFila ? 0 : c[x->atual].outro ? 99 : fonteauto_nivel(&c[x->atual], temSec);
  if (nivelAtual == FSINC_NADA) nivelAtual = 7;
  if (melhor >= 0 && atualNaFila) {
    printf("[sinc-fonte] %d de %d fonte(s) conferida(s): melhor=%d (nivel %d, %ld pontos); tocando=%d (nivel %d, %ld pontos)\n",
           feitas, n, melhor, nivel ? nivel : 7, pontos ? pontos[melhor] : 0L, x->atual, nivelAtual,
           pontos ? pontos[x->atual] : 0L);
    for (int q = 0; q < n && pontos; q++)
      if (pontos[fila[q]] > 0)
        printf("[sinc-fonte]   lista %d: fonte %d, nivel %d, %ld pontos%s%s\n", q + 1, fila[q], fonteauto_nivel(&c[fila[q]], temSec),
               pontos[fila[q]], c[fila[q]].semRef ? " (sem referencia)" : "",
               (c[fila[q]].sincPrim == FSINC_PROVAVEL || c[fila[q]].sincSec == FSINC_PROVAVEL) ? " (provavel)" : "");
  }
  fflush(stdout);
  if (vivo(x) && agoraMs() <= x->ate && melhor >= 0 && nivel == FSINC_NADA) atomic_store(&semLegendaTok, x->tok);
  // troca para a de MAIS PONTOS que esteja num nivel ESTRITAMENTE melhor que o de quem toca (quem toca tem de estar na
  // lista) E que esteja viva: sem video a legenda sincronizada nao serve, entao a seguinte da tabela entra no lugar
  if (vivo(x) && melhor >= 0 && atualNaFila && pontos && atomic_load(&jaTrocou) != x->tok) {
    unsigned char *testada = calloc((size_t)total, 1);
    int escolhida = -1;
    for (int tentativa = 0; testada && tentativa < 6 && escolhida < 0 && vivo(x); tentativa++) {
      int cand = -1, nvCand = 7;
      long melhorPts = 0;
      for (int q = 0; q < n; q++) {
        int i = fila[q], nvI = fonteauto_nivel(&c[i], temSec);
        if (i == x->atual || testada[i] || c[i].outro) continue;
        if (nvI && pontos[i] > melhorPts) { melhorPts = pontos[i]; cand = i; nvCand = nvI; }
      }
      if (cand < 0 && nivelAtual == 99 && !testada[melhor] && melhor != x->atual) { cand = melhor; nvCand = 7; }   // sem legenda em lugar nenhum, mas a que toca nem tem o original
      if (cand < 0 || nvCand >= nivelAtual) break;
      testada[cand] = 1;
      if (!fonteViva(&cena, cand)) { printf("[sinc-fonte] fonte %d (nivel %d) nao responde como video; proxima da tabela\n", cand, nvCand); fflush(stdout); continue; }
      escolhida = cand; melhor = cand; nivel = nvCand == 7 ? FSINC_NADA : nvCand;
    }
    free(testada);
    if (escolhida >= 0) {
      atomic_store(&pedidoDe, x->atual);
      atomic_store(&pedidoTok, x->tok);
      atomic_store(&pedidoSinc, c[escolhida].sincPrim == FSINC_CONFIRMADA || (temSec && c[escolhida].sincSec == FSINC_CONFIRMADA));
      atomic_store(&pedido, escolhida);
      printf("[sinc-fonte] pede a troca da fonte %d (nivel %d) para a %d (nivel %d)\n", x->atual, nivelAtual, escolhida, nivel ? nivel : 7);
      fflush(stdout);
    }
  }
fim:
  if (cena.leitor) fontesinc_leitor_destruir(cena.leitor);
  soltarDocs(&cena.docs[0]); soltarDocs(&cena.docs[1]);
  free(c); free(leg); free(pontos); free(x);
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

int fontesincapp_sem_legenda(void) {
  unsigned t = atomic_load(&semLegendaTok);
  if (!t || t != atomic_load(&vigente)) return 0;
  return atomic_compare_exchange_strong(&semLegendaTok, &t, 0);
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
