// Coletor independente da referencia do AutoSync (src/legref.c) contra MKV
// REAIS feitos por ffmpeg e mkvmerge (tests/legref_fixtures.sh).
//   bash tests/legref.sh            SANITIZE=1 / SANITIZE=thread
#include "legref.h"
#include "nvtmp.h"
#include "rede.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// legenda.c pede estes; o coletor nao usa rede de verdade aqui.
char *rede_baixar_bin(const char *url, int segundos, long *n) { (void)url; (void)segundos; (void)n; return NULL; }
unsigned rede_pedido_capacidades(void) { return REDE_CAP_JOB; }
int rede_pedir(const RedePedido *p, RedeResposta *r) { (void)p; memset(r, 0, sizeof *r); r->erro = REDE_INDISPONIVEL; return 0; }
void rede_resposta_limpar(RedeResposta *r) { free(r->corpo); free(r->cabecalhos); r->corpo = r->cabecalhos = NULL; }

static const char *DIR;
typedef struct {
  _Atomic int semRange, atrasoUs, travarApos;   // travarApos: segura o leitor depois de N pedidos
  _Atomic int liberar;
  pthread_mutex_t m; int pedidos; long long bytes; long long maiorPedido;
} Leitor;

static unsigned char *lerArquivo(void *u, const char *url, long long ini, long n, long *tam,
                                 int *status, int (*parar)(void *), void *pu) {
  Leitor *l = u; char caminho[700]; FILE *f; unsigned char *b; long long total;
  const char *nome = strrchr(url, '/');
  snprintf(caminho, sizeof caminho, "%s/%s", DIR, nome ? nome + 1 : url);
  pthread_mutex_lock(&l->m); l->pedidos++; if (n > l->maiorPedido) l->maiorPedido = n;
  int k = l->pedidos; pthread_mutex_unlock(&l->m);
  if (l->travarApos && k > l->travarApos) while (l->travarApos && !l->liberar && !parar(pu)) usleep(1000);
  if (l->atrasoUs) usleep((useconds_t)l->atrasoUs);
  if (parar(pu)) { *status = 0; return NULL; }
  f = fopen(caminho, "rb"); if (!f) { *status = 404; return NULL; }
  fseek(f, 0, SEEK_END); total = ftell(f);
  if (l->semRange) { ini = 0; n = (long)total; *status = 200; } else *status = 206;
  if (ini >= total) { fclose(f); *status = 416; return NULL; }
  if (ini + n > total) n = (long)(total - ini);
  b = malloc((size_t)n + 1); fseek(f, (long)ini, SEEK_SET);
  *tam = (long)fread(b, 1, (size_t)n, f); fclose(f);
  pthread_mutex_lock(&l->m); l->bytes += *tam; pthread_mutex_unlock(&l->m);
  return b;
}

static LegRefOrcamento orcamento(void) { return (LegRefOrcamento){ 12LL << 20, 6000, 0 }; }

static LegRefStatus esperar(LegRef *r, uint64_t id) {
  LegRefStatus s;
  for (int i = 0; i < 20000; i++) {
    s = legref_status(r);
    if (s.pedido == id && s.fase != LEGREF_LENDO) return s;
    usleep(1000);
  }
  assert(!"timeout");
  return s;
}
static LegendaDocumento *colher(LegRef *r, const char *arq, LegRefMotivo esperado, LegRefStatus *out) {
  char url[256]; uint64_t id; LegRefStatus s; LegRefOrcamento o = orcamento();
  snprintf(url, sizeof url, "https://cdn.example/x/%s", arq);
  id = legref_pedir(r, url, 7, "pt", NULL, 0, &o); assert(id);
  s = esperar(r, id);
  if (s.motivo != esperado) fprintf(stderr, "%s: motivo %s, esperado %s\n", arq, legref_motivo(s.motivo), legref_motivo(esperado));
  assert(s.motivo == esperado);
  if (out) *out = s;
  return legref_tomar(r, id);
}

int main(int argc, char **argv) {
  Leitor l; memset(&l, 0, sizeof l); pthread_mutex_init(&l.m, NULL);
  DIR = argc > 1 ? argv[1] : nvt("/nv-legref-pgs");
  LegRef *r = legref_criar(lerArquivo, &l); assert(r);
  LegendaDocumento *d; LegRefStatus s; int n; const LegendaCue *c;
  const char *bons[] = { "pgs.mkv", "pgs_simple.mkv" };   // BlockGroup e SimpleBlock
  for (int k = 0; k < 2; k++) {
    d = colher(r, bons[k], LEGREF_OK, &s); assert(d);
    assert(!strcmp(s.codec, "S_HDMV/PGS") && s.faixa == 6);
    c = legenda_documento_dados(d, &n);
    assert(n == 80);
    for (int i = 0; i < n; i++) {   // inicio da fala = bloco que MOSTRA, fim = o que LIMPA
      assert(fabs(c[i].inicio - (5.0 + i * 7.0)) < 0.0011);
      assert(fabs(c[i].fim - (5.0 + i * 7.0 + 2.5 + (i % 5) * 0.3)) < 0.0011);
    }
    { const LegendaDocumentoInfo *info = legenda_documento_info(d);
      assert((info->flags & LEGENDA_DOC_COMPLETO) && !strcmp(info->identidade, "mkv-track:6") && !strstr(info->origem, "http")); }
    assert(s.pedidos <= 4 + 16);   // cabeca, Cues e 16 amostras pequenas
    legenda_documento_liberar(d);
    printf("%s: %d falas, %d pedidos, %lld bytes\n", bons[k], n, s.pedidos, s.bytes);
  }
  d = colher(r, "pgs_buraco.mkv", LEGREF_INCOMPLETO, NULL); assert(!d);   // paridade quebrada: recusa
  d = colher(r, "pgs_curto.mkv", LEGREF_INCOMPLETO, NULL); assert(!d);
  // Faixa de TEXTO (sem BlockDuration no fixture) segue o caminho de sempre: recusa pelo motivo antigo.
  d = colher(r, "texto.mkv", LEGREF_SEM_DURACAO, &s); assert(!d && !strcmp(s.codec, "S_TEXT/UTF8"));
  legref_destruir(r);
  printf("legref pgs ok\n");
  return 0;
}
