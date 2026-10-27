// TESTE DO IDIOMA ORIGINAL EM MASSA (dono, 2026-10-26): roda a cadeia de idiomaorig.c — o mesmo codigo
// do aparelho — sobre uma lista de titulos e grava, por titulo, a resposta de cada fonte, o tempo e a
// decisao. A comparacao com a verdade e o resumo ficam no script (tests/idiomaorig_amostra.py).
// Entrada: TSV  imdb<TAB>serie(0/1)   Saida: TSV  imdb tmdb wd wdTodas falado paisTmdb cinemeta final fonte ms...
// Uso: idiomaorig_amostra <entrada.tsv> <saida.tsv> <chave_tmdb> [fios]
#include "idiomaorig.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char imdb[24]; int serie; IdiomaOrig r; } Item;
static Item *itens;
static int nItens, prox;
static const char *chave;
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;

static void *fio(void *u) {
  (void)u;
  for (;;) {
    int i;
    pthread_mutex_lock(&trava); i = prox++; pthread_mutex_unlock(&trava);
    if (i >= nItens) return NULL;
    idiomaorig_consultar(itens[i].imdb, itens[i].serie, chave, &itens[i].r);
    if (i % 100 == 0) { fprintf(stderr, "%d/%d\n", i, nItens); }
  }
}

int main(int argc, char **argv) {
  FILE *f, *o;
  char linha[128];
  int cap = 2048, nf, i;
  pthread_t t[32];
  // --decidir saida.tsv nova.tsv: refaz SO a decisao sobre respostas ja gravadas (sem rede), para
  // comparar regras de decisao sobre exatamente os mesmos dados.
  if (argc == 4 && !strcmp(argv[1], "--decidir")) {
    char l[512];
    FILE *e = fopen(argv[2], "r"), *s = fopen(argv[3], "w");
    if (!e || !s) { perror("--decidir"); return 1; }
    if (fgets(l, sizeof l, e)) fputs(l, s);
    while (fgets(l, sizeof l, e)) {
      char *c[13] = {0}, *p = l;
      IdiomaOrig r;
      int k;
      memset(&r, 0, sizeof r);
      for (k = 0; k < 13 && p; k++) { c[k] = p; p = strchr(p, '\t'); if (p) *p++ = 0; }
      if (k < 13) continue;
      c[12][strcspn(c[12], "\n")] = 0;
      snprintf(r.lang[IO_TMDB_FIND], 8, "%s", c[1]); snprintf(r.lang[IO_WIKIDATA], 8, "%s", c[2]);
      snprintf(r.wdTodas, sizeof r.wdTodas, "%s", c[3]); snprintf(r.lang[IO_TMDB_FALADO], 8, "%s", c[4]);
      snprintf(r.lang[IO_TMDB_PAIS], 8, "%s", c[5]); snprintf(r.lang[IO_CINEMETA_PAIS], 8, "%s", c[6]);
      idiomaorig_decidir(&r);
      fprintf(s, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%s\t%s\t%s\t%s\n", c[0], c[1], c[2], c[3], c[4], c[5], c[6],
              r.final, r.fonteFinal, c[9], c[10], c[11], c[12]);
    }
    fclose(e); fclose(s);
    return 0;
  }
  if (argc < 4) { fprintf(stderr, "uso: %s entrada.tsv saida.tsv chave_tmdb [fios]\n", argv[0]); return 2; }
  chave = argv[3];
  nf = argc > 4 ? atoi(argv[4]) : 8;
  if (nf < 1 || nf > 32) nf = 8;
  SDL_Init(SDL_INIT_TIMER);
  itens = calloc((size_t)cap, sizeof *itens);
  f = fopen(argv[1], "r");
  if (!f) { perror(argv[1]); return 1; }
  while (fgets(linha, sizeof linha, f) && nItens < cap) {
    char *tab = strchr(linha, '\t');
    if (!tab) continue;
    *tab = 0;
    snprintf(itens[nItens].imdb, sizeof itens[nItens].imdb, "%s", linha);
    itens[nItens].serie = atoi(tab + 1);
    nItens++;
  }
  fclose(f);
  for (i = 0; i < nf; i++) pthread_create(&t[i], NULL, fio, NULL);
  for (i = 0; i < nf; i++) pthread_join(t[i], NULL);
  o = fopen(argv[2], "w");
  if (!o) { perror(argv[2]); return 1; }
  fprintf(o, "imdb\ttmdb\twd\twdTodas\tfalado\tpaisTmdb\tcinemeta\tfinal\tfonte\tms_tmdb\tms_wd\tms_det\tms_cine\n");
  for (i = 0; i < nItens; i++) {
    IdiomaOrig *r = &itens[i].r;
    fprintf(o, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\n", itens[i].imdb,
            r->lang[IO_TMDB_FIND], r->lang[IO_WIKIDATA], r->wdTodas, r->lang[IO_TMDB_FALADO],
            r->lang[IO_TMDB_PAIS], r->lang[IO_CINEMETA_PAIS], r->final, r->fonteFinal,
            r->ms[IO_TMDB_FIND], r->ms[IO_WIKIDATA], r->ms[IO_TMDB_FALADO], r->ms[IO_CINEMETA_PAIS]);
  }
  fclose(o);
  fprintf(stderr, "ok: %d titulos\n", nItens);
  return 0;
}
