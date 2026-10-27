// TESTE DE LEGENDAS PT-PT EM MASSA (dono, 2026-10-26: "legenda em PT nao pode falhar").
// Por titulo: pede a lista ao addon de legendas (Stremio, JSON), classifica cada item com o codigo do app
// (ling_normalizar + ling_casa contra "pt" = Portugal), escolhe a legenda como o app escolhe
// (legauto_escolher), baixa e le com o parser do app (legenda_utf8 + legenda_extrair). Para medir a
// sincronia sem o video, procura a legenda INGLESA do mesmo release (mesmo nome de arquivo/release) e grava
// os tempos das duas; o alinhamento e feito por tests/legendas_amostra.py.
// Uso: legendas_amostra <entrada.tsv imdb> <dir_saida> <url_base_addon> [fios]
#include "addons.h"
#include "js.h"
#include "legauto.h"
#include "legenda.h"
#include "linguas.h"
#include "rede.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXL 160
typedef struct { char imdb[24]; char linha[1024]; } Item;
static Item *itens;
static int nItens, prox;
static const char *dir, *base;
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;

// Baixa e le uma legenda com o parser do app; grava "inicio fim" por bloco. Devolve n de blocos (-1 falha).
static int baixarCues(const char *url, const char *arquivo) {
  long n = 0;
  char *bytes = rede_baixar_bin(url, 20, &n), *txt;
  LegendaCue *cues = NULL;
  int k, i;
  FILE *f;
  if (!bytes) return -1;
  txt = legenda_utf8(bytes, n, NULL);
  free(bytes);
  if (!txt) return -1;
  k = legenda_extrair(txt, &cues);
  free(txt);
  if (k <= 0) { free(cues); return 0; }
  f = fopen(arquivo, "w");
  if (f) { for (i = 0; i < k; i++) fprintf(f, "%.3f %.3f\n", cues[i].inicio, cues[i].fim); fclose(f); }
  free(cues);
  return k;
}

static void avaliar(Item *it) {
  char url[400], *corpo;
  const char *q;
  Legenda *pt = calloc(MAXL, sizeof *pt), *en = calloc(MAXL, sizeof *en);
  int nPt = 0, nBr = 0, nEn = 0, escolhida, ref = -1, melhor = -1, k;
  char arq[600];
  snprintf(url, sizeof url, "%s/subtitles/movie/%s.json", base, it->imdb);
  corpo = rede_baixar(url, 20);
  if (!corpo) { snprintf(it->linha, sizeof it->linha, "%s\tsem_resposta\t0\t0\t0\t\t\t0\t-1\t-1", it->imdb); free(pt); free(en); return; }
  for (q = js_array(corpo, NULL, "subtitles"); q; q = js_prox(js_fim(q))) {
    const char *f = js_fim(q);
    char l[64] = "", cod[16], nome[120] = "", link[1024] = "";
    js_texto(q, f, "lang", l, sizeof l);
    ling_normalizar(l, cod, sizeof cod);
    js_texto(q, f, "url", link, sizeof link);
    js_texto(q, f, "subtitleFileName", nome, sizeof nome);
    if (!nome[0]) js_texto(q, f, "movieReleaseName", nome, sizeof nome);
    if (!link[0]) continue;
    if (ling_casa(cod, "pt-br") && ling_casa("pt-br", cod) && strcmp(cod, "pt") && strcmp(cod, "por")) nBr++;
    if (ling_casa(cod, "pt") && nPt < MAXL) {          // o que o app aceita para "Portugues (Portugal)"
      Legenda *d = &pt[nPt++];
      snprintf(d->idioma, sizeof d->idioma, "%s", cod); snprintf(d->url, sizeof d->url, "%s", link);
      snprintf(d->arquivo, sizeof d->arquivo, "%s", nome);
    } else if (ling_casa(cod, "en") && nEn < MAXL) {
      Legenda *d = &en[nEn++];
      snprintf(d->idioma, sizeof d->idioma, "%s", cod); snprintf(d->url, sizeof d->url, "%s", link);
      snprintf(d->arquivo, sizeof d->arquivo, "%s", nome);
    }
  }
  free(corpo);
  escolhida = nPt ? legauto_escolher(pt, nPt, "pt", "", NULL, 0, 0) : -1;
  if (escolhida >= 0) {
    // referencia: a inglesa com mais palavras de release em comum com a escolhida
    for (k = 0; k < nEn; k++) {
      int a = legauto_afinidade_release(en[k].arquivo, pt[escolhida].arquivo);
      if (a > melhor) { melhor = a; ref = k; }
    }
  }
  {
    int cPt = -1, cEn = -1;
    if (escolhida >= 0) {
      snprintf(arq, sizeof arq, "%s/%s.pt.txt", dir, it->imdb);
      cPt = baixarCues(pt[escolhida].url, arq);
    }
    if (ref >= 0) {
      snprintf(arq, sizeof arq, "%s/%s.en.txt", dir, it->imdb);
      cEn = baixarCues(en[ref].url, arq);
    }
    snprintf(it->linha, sizeof it->linha, "%s\tok\t%d\t%d\t%d\t%s\t%s\t%d\t%d\t%d", it->imdb, nPt, nBr, nEn,
             escolhida >= 0 ? pt[escolhida].arquivo : "", ref >= 0 ? en[ref].arquivo : "", melhor, cPt, cEn);
  }
  free(pt); free(en);
}

static void *fio(void *u) {
  (void)u;
  for (;;) {
    int i;
    pthread_mutex_lock(&trava); i = prox++; pthread_mutex_unlock(&trava);
    if (i >= nItens) return NULL;
    avaliar(&itens[i]);
  }
}

int main(int argc, char **argv) {
  FILE *f;
  char l[128], s[600];
  int nf, i;
  pthread_t t[16];
  if (argc < 4) { fprintf(stderr, "uso: %s entrada.tsv dir_saida url_base_addon [fios]\n", argv[0]); return 2; }
  dir = argv[2]; base = argv[3]; nf = argc > 4 ? atoi(argv[4]) : 6;
  if (nf < 1 || nf > 16) nf = 6;
  SDL_Init(SDL_INIT_TIMER);
  itens = calloc(1024, sizeof *itens);
  f = fopen(argv[1], "r");
  if (!f) { perror(argv[1]); return 1; }
  while (fgets(l, sizeof l, f) && nItens < 1024) {
    l[strcspn(l, "\t\r\n")] = 0;
    if (l[0]) snprintf(itens[nItens++].imdb, sizeof itens[0].imdb, "%s", l);
  }
  fclose(f);
  for (i = 0; i < nf; i++) pthread_create(&t[i], NULL, fio, NULL);
  for (i = 0; i < nf; i++) pthread_join(t[i], NULL);
  snprintf(s, sizeof s, "%s/resumo.tsv", dir);
  f = fopen(s, "w");
  if (!f) { perror(s); return 1; }
  fprintf(f, "imdb\testado\tn_pt\tn_ptbr\tn_en\tescolhida\treferencia\tafinidade\tcues_pt\tcues_en\n");
  for (i = 0; i < nItens; i++) fprintf(f, "%s\n", itens[i].linha);
  fclose(f);
  fprintf(stderr, "ok: %d titulos\n", nItens);
  return 0;
}
