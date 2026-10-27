// A CONFERENCIA REAL de uma fonte contra uma legenda (fontesinc.c): MKV de verdade (ffmpeg + mkvmerge, os mesmos
// fixtures do legref/legsync), a legref e a engine do AutoSync de verdade. So a rede e trocada por um leitor de arquivo.
//
//   bash tests/fontesinc.sh
#include "fontesinc.h"
#include "fonteauto.h"
#include "rede.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

// A rede de verdade fica de fora: o leitor abaixo le do disco (como tests/legref.c).
char *rede_baixar_bin(const char *url, int segundos, long *n) { (void)url; (void)segundos; (void)n; return NULL; }
unsigned rede_pedido_capacidades(void) { return REDE_CAP_JOB; }
int rede_pedir(const RedePedido *p, RedeResposta *r) { (void)p; memset(r, 0, sizeof *r); r->erro = REDE_INDISPONIVEL; return 0; }
void rede_resposta_limpar(RedeResposta *r) { free(r->corpo); free(r->cabecalhos); r->corpo = r->cabecalhos = NULL; }
const char *i18n(const char *s) { return s; }

static const char *DIR;
static int pedidos;
static pthread_mutex_t M = PTHREAD_MUTEX_INITIALIZER;

static unsigned char *ler(void *u, const char *url, long long ini, long n, long *tam, int *status,
                          int (*parar)(void *), void *pu) {
  char caminho[700]; FILE *f; unsigned char *b; long long total;
  const char *nome = strrchr(url, '/');
  (void)u;
  snprintf(caminho, sizeof caminho, "%s/%s", DIR, nome ? nome + 1 : url);
  pthread_mutex_lock(&M); pedidos++; pthread_mutex_unlock(&M);
  if (parar(pu)) { *status = 0; return NULL; }
  f = fopen(caminho, "rb");
  if (!f) { *status = 404; return NULL; }
  fseek(f, 0, SEEK_END); total = ftell(f);
  *status = 206;
  if (ini >= total) { fclose(f); *status = 416; return NULL; }
  if (ini + n > total) n = (long)(total - ini);
  b = malloc((size_t)n + 1); fseek(f, (long)ini, SEEK_SET);
  *tam = (long)fread(b, 1, (size_t)n, f); fclose(f);
  return b;
}

static LegendaDocumento *legenda(const char *nome, uint64_t sessao) {
  char caminho[700]; FILE *f; long n; char *b; LegendaDocumento *d;
  LegendaDocumentoInfo i = { .sessao = sessao, .flags = LEGENDA_DOC_COMPLETO };
  snprintf(i.idioma, sizeof i.idioma, "pt");
  snprintf(caminho, sizeof caminho, "%s/%s", DIR, nome);
  f = fopen(caminho, "rb"); assert(f);
  fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
  b = malloc((size_t)n + 1); assert(fread(b, 1, (size_t)n, f) == (size_t)n); b[n] = 0; fclose(f);
  d = legenda_documento_criar(b, &i); free(b); assert(d);
  return d;
}

static int cancelaJa(void *u) { return *(int *)u; }

#define SESSAO 7u
int main(int c, char **v) {
  FonteSincLeitor *l;
  LegendaDocumento *mais, *menos, *ruim;
  int off, r, antes, nao = 0;
  assert(c > 1);
  DIR = v[1];
  l = fontesinc_leitor_criar(ler, NULL); assert(l);
  mais = legenda("ext_mais2500.srt", SESSAO);
  menos = legenda("ext_menos1200.srt", SESSAO);
  ruim = legenda("ext_ruim.srt", SESSAO);

  // sincroniza com a fonte (a faixa embutida em ingles, a legenda 2,5 s adiantada): CONFIRMADA e o offset certo
  r = fontesinc_julgar(l, "http://fonte/ff.mkv", SESSAO, "en", mais, NULL, NULL, &off);
  printf("mais2500: %d off=%d motivo=%s pedidos=%d\n", r, off, fontesinc_motivo(l), pedidos);
  assert(r == FSINC_CONFIRMADA && off > 2300 && off < 2700);
  // a MESMA fonte, outra legenda: a referencia guardada, nenhuma leitura nova
  antes = pedidos;
  { struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
    r = fontesinc_julgar(l, "http://fonte/ff.mkv", SESSAO, "en", menos, NULL, NULL, &off);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("comparacao com a referencia guardada: %.1f ms\n", (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6); }
  printf("menos1200: %d off=%d motivo=%s pedidos novos=%d\n", r, off, fontesinc_motivo(l), pedidos - antes);
  assert(r == FSINC_CONFIRMADA && off < -1000 && off > -1400);
  assert(pedidos == antes);
  // legenda que NAO sincroniza com a fonte: NAO (e continua sem ler de novo)
  { struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
    r = fontesinc_julgar(l, "http://fonte/ff.mkv", SESSAO, "en", ruim, NULL, NULL, &off);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("recusa (o caso lento): %.1f ms\n", (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6); }
  printf("ruim: %d motivo=%s\n", r, fontesinc_motivo(l));
  assert(r == FSINC_NAO && pedidos == antes);

  // OUTRA fonte (outro arquivo): le a referencia dela
  antes = pedidos;
  r = fontesinc_julgar(l, "http://fonte/filme.mkv", SESSAO, "en", legenda("filme_ext_mais2500.srt", SESSAO), NULL, NULL, &off);
  printf("filme: %d off=%d motivo=%s pedidos novos=%d\n", r, off, fontesinc_motivo(l), pedidos - antes);
  assert(r == FSINC_CONFIRMADA && off > 2300 && off < 2700 && pedidos > antes);

  // A FAIXA EMBUTIDA: em ingles so tem a faixa inglesa; a que tem uma faixa em portugues entrega o idioma dela
  { char idioma[24]; int dialogo = 0;
    r = fontesinc_faixa(l, "http://fonte/ff.mkv", SESSAO, "por", NULL, NULL, idioma, sizeof idioma, &dialogo);
    printf("faixa de ff.mkv (pedindo por): r=%d idioma=%s dialogo=%d\n", r, idioma, dialogo);
    assert(r == 1 && dialogo == 1 && strncmp(idioma, "po", 2) && strncmp(idioma, "pt", 2));
    r = fontesinc_faixa(l, "http://fonte/cruz.mkv", SESSAO, "por", NULL, NULL, idioma, sizeof idioma, &dialogo);
    printf("faixa de cruz.mkv (pedindo por): r=%d idioma=%s dialogo=%d\n", r, idioma, dialogo);
    assert(r == 1 && dialogo == 1 && (!strncmp(idioma, "po", 2) || !strncmp(idioma, "pt", 2)));
    r = fontesinc_faixa(l, "http://fonte/vob.mkv", SESSAO, "por", NULL, NULL, idioma, sizeof idioma, &dialogo);
    assert(r == 0 && !idioma[0] && !dialogo);   // so imagem: nenhuma faixa de texto
    r = fontesinc_faixa(l, "http://fonte/nao-existe.mkv", SESSAO, "por", NULL, NULL, idioma, sizeof idioma, &dialogo);
    assert(r == 0); }

  // fonte SEM faixa de texto aproveitavel (so imagem): NAO, com o motivo; e fonte que nao existe (404): NAO
  r = fontesinc_julgar(l, "http://fonte/vob.mkv", SESSAO, "en", mais, NULL, NULL, &off);
  printf("sem faixa de texto: %d motivo=%s\n", r, fontesinc_motivo(l));
  assert(r == FSINC_NAO);
  r = fontesinc_julgar(l, "http://fonte/nao-existe.mkv", SESSAO, "en", mais, NULL, NULL, &off);
  printf("404: %d motivo=%s\n", r, fontesinc_motivo(l));
  assert(r == FSINC_NAO);
  nao++;

  // cancelado antes de ler: nem confirma nem nega
  { int cancela = 1;
    r = fontesinc_julgar(l, "http://fonte/cruz2.mkv", SESSAO, "en", mais, cancelaJa, &cancela, &off);
    printf("cancelado: %d\n", r);
    assert(r == FSINC_DESCONHECIDA); }

  // entradas invalidas
  assert(fontesinc_julgar(NULL, "x", SESSAO, "en", mais, NULL, NULL, NULL) == FSINC_NAO);
  assert(fontesinc_julgar(l, "", SESSAO, "en", mais, NULL, NULL, NULL) == FSINC_NAO);
  assert(fontesinc_julgar(l, "http://fonte/ff.mkv", SESSAO, "en", NULL, NULL, NULL, NULL) == FSINC_NAO);

  legenda_documento_liberar(mais); legenda_documento_liberar(menos);
  legenda_documento_liberar(ruim);
  fontesinc_leitor_destruir(l);
  printf("fontesinc: ok (%d)\n", nao);
  return 0;
}
