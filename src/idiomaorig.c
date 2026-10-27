// Idioma original de um titulo: ver idiomaorig.h.
#include "idiomaorig.h"
#include "js.h"
#include "rede.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define IO_PRAZO_S 6

// Pais -> idioma principal. Paises com mais de um idioma corrente no cinema (India, Suica, Belgica,
// Canada, Espanha com catalao...) ficam de FORA de proposito: um chute ali erraria o audio.
static const struct { const char *iso, *nome, *lang; } PAISES[] = {
  {"US","United States","en"},{"GB","United Kingdom","en"},{"GB","UK","en"},{"AU","Australia","en"},
  {"NZ","New Zealand","en"},{"IE","Ireland","en"},{"FR","France","fr"},{"DE","Germany","de"},
  {"AT","Austria","de"},{"IT","Italy","it"},{"ES","Spain","es"},{"MX","Mexico","es"},
  {"AR","Argentina","es"},{"CO","Colombia","es"},{"CL","Chile","es"},{"PE","Peru","es"},
  {"PT","Portugal","pt"},{"BR","Brazil","pt"},{"JP","Japan","ja"},{"KR","South Korea","ko"},
  {"KR","Korea","ko"},{"CN","China","zh"},{"TW","Taiwan","zh"},{"HK","Hong Kong","cn"},
  {"RU","Russia","ru"},{"TR","Turkey","tr"},{"DK","Denmark","da"},{"SE","Sweden","sv"},
  {"NO","Norway","no"},{"FI","Finland","fi"},{"IS","Iceland","is"},{"PL","Poland","pl"},
  {"CZ","Czech Republic","cs"},{"CZ","Czechia","cs"},{"SK","Slovakia","sk"},{"HU","Hungary","hu"},
  {"RO","Romania","ro"},{"GR","Greece","el"},{"NL","Netherlands","nl"},{"TH","Thailand","th"},
  {"IR","Iran","fa"},{"EG","Egypt","ar"},{"SA","Saudi Arabia","ar"},{"LB","Lebanon","ar"},
  {"ID","Indonesia","id"},{"PH","Philippines","tl"},{"VN","Vietnam","vi"},{"IL","Israel","he"},
  {"UA","Ukraine","uk"},{"RS","Serbia","sr"},{"HR","Croatia","hr"},{"BG","Bulgaria","bg"},
  {"EE","Estonia","et"},{"LV","Latvia","lv"},{"LT","Lithuania","lt"},{"GE","Georgia","ka"},
  {"MY","Malaysia","ms"},{"PK","Pakistan","ur"},{"BD","Bangladesh","bn"},
};

const char *idiomaorig_do_pais(const char *pais) {
  size_t i;
  if (!pais || !pais[0]) return "";
  for (i = 0; i < sizeof PAISES / sizeof *PAISES; i++)
    if (!strcasecmp(pais, PAISES[i].iso) || !strcasecmp(pais, PAISES[i].nome)) return PAISES[i].lang;
  return "";
}

static void urlCodificar(const char *s, char *d, size_t n) {
  static const char hex[] = "0123456789ABCDEF";
  size_t o = 0;
  for (; *s && o + 4 < n; s++) {
    unsigned char c = (unsigned char)*s;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') d[o++] = (char)c;
    else { d[o++] = '%'; d[o++] = hex[c >> 4]; d[o++] = hex[c & 15]; }
  }
  d[o] = 0;
}

static int imdbValido(const char *imdb) {
  size_t i;
  if (strncmp(imdb, "tt", 2)) return 0;
  for (i = 2; imdb[i] && imdb[i] != ':'; i++) if (!isdigit((unsigned char)imdb[i])) return 0;
  return i > 2;
}

// TMDB /find: um pedido devolve o original_language e o id do TMDB.
static void tmdbFind(const char *imdb, int serie, const char *chave, IdiomaOrig *r) {
  char url[400];
  char *corpo;
  const char *v;
  Uint32 t0 = SDL_GetTicks();
  snprintf(url, sizeof url, "https://api.themoviedb.org/3/find/%.*s?api_key=%s&external_source=imdb_id",
           (int)strcspn(imdb, ":"), imdb, chave);
  corpo = rede_baixar(url, IO_PRAZO_S);
  r->ms[IO_TMDB_FIND] = (int)(SDL_GetTicks() - t0);
  if (!corpo) return;
  v = js_array(corpo, NULL, serie ? "tv_results" : "movie_results");
  if (!v) v = js_array(corpo, NULL, serie ? "movie_results" : "tv_results");   // tipo trocado no catalogo
  if (v) {
    const char *f = js_fim(v);
    js_texto(v, f, "original_language", r->lang[IO_TMDB_FIND], sizeof r->lang[IO_TMDB_FIND]);
    r->tmdbId = (long)js_num(v, f, "id", 0.0);
  }
  free(corpo);
}

// TMDB /movie|tv/<id>: o primeiro idioma falado e o pais de producao (ou de origem, na serie).
static void tmdbDetalhes(int serie, const char *chave, IdiomaOrig *r) {
  char url[300];
  char *corpo;
  const char *v;
  Uint32 t0 = SDL_GetTicks();
  if (r->tmdbId <= 0) return;
  snprintf(url, sizeof url, "https://api.themoviedb.org/3/%s/%ld?api_key=%s", serie ? "tv" : "movie",
           r->tmdbId, chave);
  corpo = rede_baixar(url, IO_PRAZO_S);
  r->ms[IO_TMDB_FALADO] = r->ms[IO_TMDB_PAIS] = (int)(SDL_GetTicks() - t0);
  if (!corpo) return;
  v = js_array(corpo, NULL, "spoken_languages");
  if (v) js_texto(v, js_fim(v), "iso_639_1", r->lang[IO_TMDB_FALADO], sizeof r->lang[IO_TMDB_FALADO]);
  v = js_array(corpo, NULL, "production_countries");
  if (v) {
    char iso[8] = "";
    js_texto(v, js_fim(v), "iso_3166_1", iso, sizeof iso);
    snprintf(r->lang[IO_TMDB_PAIS], sizeof r->lang[IO_TMDB_PAIS], "%s", idiomaorig_do_pais(iso));
  } else if ((v = js_array(corpo, NULL, "origin_country")) != NULL) {
    char iso[8] = "";
    js_cadeia(v, iso, sizeof iso);
    snprintf(r->lang[IO_TMDB_PAIS], sizeof r->lang[IO_TMDB_PAIS], "%s", idiomaorig_do_pais(iso));
  }
  free(corpo);
}

// Wikidata: "idioma original do filme ou programa" (P364) do item com aquele IMDb (P345).
static void wikidata(const char *imdb, IdiomaOrig *r) {
  char q[400], qc[1200], url[1500];
  char *corpo;
  const char *b;
  Uint32 t0 = SDL_GetTicks();
  snprintf(q, sizeof q,
           "SELECT ?c WHERE { ?f wdt:P345 \"%.*s\"; wdt:P364 ?l. ?l wdt:P218 ?c. }",
           (int)strcspn(imdb, ":"), imdb);
  urlCodificar(q, qc, sizeof qc);
  snprintf(url, sizeof url, "https://query.wikidata.org/sparql?format=json&query=%s", qc);
  corpo = rede_baixar(url, IO_PRAZO_S);
  r->ms[IO_WIKIDATA] = (int)(SDL_GetTicks() - t0);
  if (!corpo) return;
  for (b = js_array(corpo, NULL, "bindings"); b; b = js_prox(js_fim(b))) {
    char c[8] = "";
    const char *f = js_fim(b);
    js_texto(b, f, "value", c, sizeof c);
    if (!c[0]) continue;
    if (!strcmp(c, "yue")) snprintf(c, sizeof c, "cn");   // TMDB chama o cantones de "cn"
    if (!r->lang[IO_WIKIDATA][0]) snprintf(r->lang[IO_WIKIDATA], sizeof r->lang[IO_WIKIDATA], "%s", c);
    if (strlen(r->wdTodas) + strlen(c) + 2 < sizeof r->wdTodas) {
      if (r->wdTodas[0]) strcat(r->wdTodas, ",");
      strcat(r->wdTodas, c);
    }
  }
  free(corpo);
}

// Cinemeta: o campo "country" ("United States, United Kingdom") -> o primeiro pais com idioma unico.
static void cinemeta(const char *imdb, int serie, IdiomaOrig *r) {
  char url[300], paises[200] = "";
  char *corpo, *p, *sv;
  Uint32 t0 = SDL_GetTicks();
  snprintf(url, sizeof url, "https://v3-cinemeta.strem.io/meta/%s/%.*s.json", serie ? "series" : "movie",
           (int)strcspn(imdb, ":"), imdb);
  corpo = rede_baixar(url, IO_PRAZO_S);
  r->ms[IO_CINEMETA_PAIS] = (int)(SDL_GetTicks() - t0);
  if (!corpo) return;
  js_texto(corpo, NULL, "country", paises, sizeof paises);
  free(corpo);
  for (p = strtok_r(paises, ",", &sv); p; p = strtok_r(NULL, ",", &sv)) {
    while (*p == ' ') p++;
    if (idiomaorig_do_pais(p)[0]) {
      snprintf(r->lang[IO_CINEMETA_PAIS], sizeof r->lang[IO_CINEMETA_PAIS], "%s", idiomaorig_do_pais(p));
      break;
    }
  }
}

// A DECISAO, medida em 1008 filmes (tests/idiomaorig_amostra): o original_language do TMDB acerta 99,2%;
// votar com os sinais fracos (pais, idioma falado, a lista do Wikidata) PIORAVA para 98,9%. O erro tipico
// do TMDB e marcar o idioma do PAIS DE PRODUCAO num filme rodado em outra lingua (Dogman, Demons,
// Resident Evil Vendetta). Por isso: TMDB, e so troca quando o Wikidata da UM idioma so, diferente, e o
// proprio TMDB diz que esse e o idioma falado.
void idiomaorig_decidir(IdiomaOrig *r) {
  const char *t = r->lang[IO_TMDB_FIND], *w = r->lang[IO_WIKIDATA];
  r->final[0] = 0; r->fonteFinal = -1;
  if (t[0] && w[0] && strcmp(t, w) && !strchr(r->wdTodas, ',') && !strcmp(r->lang[IO_TMDB_FALADO], w)) {
    snprintf(r->final, sizeof r->final, "%s", w); r->fonteFinal = IO_WIKIDATA;
    return;
  }
  if (t[0]) { snprintf(r->final, sizeof r->final, "%s", t); r->fonteFinal = IO_TMDB_FIND; return; }
  if (w[0]) { snprintf(r->final, sizeof r->final, "%s", w); r->fonteFinal = IO_WIKIDATA; return; }
  for (int f = IO_TMDB_FALADO; f < IO_N; f++)
    if (r->lang[f][0]) { snprintf(r->final, sizeof r->final, "%s", r->lang[f]); r->fonteFinal = f; return; }
}

void idiomaorig_consultar(const char *imdb, int serie, const char *chaveTmdb, IdiomaOrig *r) {
  memset(r, 0, sizeof *r);
  if (!imdb || !imdbValido(imdb)) return;
  if (chaveTmdb && chaveTmdb[0]) { tmdbFind(imdb, serie, chaveTmdb, r); tmdbDetalhes(serie, chaveTmdb, r); }
  wikidata(imdb, r);
  cinemeta(imdb, serie, r);
  idiomaorig_decidir(r);
}

// ---- No aparelho: tabela na memoria + arquivo + dois fios. ----
// TODO TITULO QUE APARECE TEM IDIOMA (dono, 2026-10-27: "se vejo o nome e o poster, tenho de ter o idioma
// original"). O catalogo pede os de cada lote que chega (idiomaorig_pedir, fila de FUNDO, na ordem); o que a
// pessoa olha (destaque, ficha, Play) vai na fila URGENTE, que passa na frente. Um titulo e consultado uma vez
// na vida da TV: a resposta final fica no arquivo.
#define IO_SLOTS 8192   // potencia de 2; cheia, os titulos novos so ficam sem cache (consulta de novo)
#define IO_URG 32
#define IO_FUNDO 2048
#define IO_FIOS 2
typedef struct { char id[24]; char lang[8]; unsigned char estado; } IoSlot;   // estado: 0 vazio, 1 na fila, 2 provisorio, 3 final
typedef struct { char id[24]; int serie; } IoPedido;
static IoSlot ioTab[IO_SLOTS];
static pthread_mutex_t ioTrava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ioSinal = PTHREAD_COND_INITIALIZER;
static IoPedido ioUrg[IO_URG], ioFundo[IO_FUNDO];
static int ioUrgN, ioFundoIni, ioFundoN, ioFios;
static char ioArquivo[512], ioChaveTmdb[80];

// A chave do titulo: o IMDb sem o episodio ("tt123"), ou "tmdb:N" para quem so tem o id do TMDB.
static int ioChave(const char *imdb, long tmdb, char id[24]) {
  id[0] = 0;
  if (imdb && imdbValido(imdb)) {
    size_t n = strcspn(imdb, ":");
    if (n > 23) n = 23;
    memcpy(id, imdb, n); id[n] = 0;
  } else if (tmdb > 0) snprintf(id, 24, "tmdb:%d", (int)tmdb);
  else if (imdb && !strncmp(imdb, "tmdb:", 5)) {
    const char *p = imdb + 5;
    if (*p == 't' || *p == 'm') p++;   // "tmdb:t1399"
    if (atol(p) > 0) snprintf(id, 24, "tmdb:%d", (int)atol(p));
  }
  return id[0] != 0;
}
// Com a trava. NULL se a tabela esta cheia.
static IoSlot *ioSlot(const char *id) {
  unsigned h = 2166136261u;
  for (const char *p = id; *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
  for (unsigned i = 0; i < IO_SLOTS; i++) {
    IoSlot *s = &ioTab[(h + i) & (IO_SLOTS - 1)];
    if (!s->id[0]) { snprintf(s->id, sizeof s->id, "%s", id); return s; }
    if (!strcmp(s->id, id)) return s;
  }
  return NULL;
}

// Wikidata pelo id do TMDB (P4947 filme, P4983 serie), para quem nao tem IMDb.
static void wikidataTmdb(long tmdb, int serie, IdiomaOrig *r) {
  char q[400], qc[1200], url[1500];
  char *corpo;
  const char *b;
  Uint32 t0 = SDL_GetTicks();
  snprintf(q, sizeof q, "SELECT ?c WHERE { ?f wdt:%s \"%ld\"; wdt:P364 ?l. ?l wdt:P218 ?c. }",
           serie ? "P4983" : "P4947", tmdb);
  urlCodificar(q, qc, sizeof qc);
  snprintf(url, sizeof url, "https://query.wikidata.org/sparql?format=json&query=%s", qc);
  corpo = rede_baixar(url, IO_PRAZO_S);
  r->ms[IO_WIKIDATA] = (int)(SDL_GetTicks() - t0);
  if (!corpo) return;
  for (b = js_array(corpo, NULL, "bindings"); b; b = js_prox(js_fim(b))) {
    char c[8] = "";
    js_texto(b, js_fim(b), "value", c, sizeof c);
    if (!c[0]) continue;
    if (!strcmp(c, "yue")) snprintf(c, sizeof c, "cn");
    if (!r->lang[IO_WIKIDATA][0]) snprintf(r->lang[IO_WIKIDATA], sizeof r->lang[IO_WIKIDATA], "%s", c);
    if (strlen(r->wdTodas) + strlen(c) + 2 < sizeof r->wdTodas) {
      if (r->wdTodas[0]) strcat(r->wdTodas, ",");
      strcat(r->wdTodas, c);
    }
  }
  free(corpo);
}
// So o original_language da ficha do TMDB (para "tmdb:N", que nao passa pelo /find).
static void tmdbOriginal(long tmdb, int serie, const char *chave, IdiomaOrig *r) {
  char url[300];
  char *corpo;
  Uint32 t0 = SDL_GetTicks();
  snprintf(url, sizeof url, "https://api.themoviedb.org/3/%s/%ld?api_key=%s", serie ? "tv" : "movie", tmdb, chave);
  corpo = rede_baixar(url, IO_PRAZO_S);
  r->ms[IO_TMDB_FIND] = (int)(SDL_GetTicks() - t0);
  if (!corpo) return;
  js_texto_raiz(corpo, "original_language", r->lang[IO_TMDB_FIND], sizeof r->lang[IO_TMDB_FIND]);
  r->tmdbId = tmdb;
  free(corpo);
}

static void ioGuardar(const char *id, const char *lang, int final) {
  IoSlot *s;
  pthread_mutex_lock(&ioTrava);
  if ((s = ioSlot(id)) != NULL && s->estado < 3) {
    snprintf(s->lang, sizeof s->lang, "%s", lang);
    // Sem resposta nenhuma (rede caida) nao vira final: a proxima vez consulta de novo.
    s->estado = final ? (lang[0] ? 3 : 0) : 2;
    if (final && lang[0] && ioArquivo[0]) {
      FILE *f = fopen(ioArquivo, "a");
      if (f) { fprintf(f, "%s\t%s\n", id, lang); fclose(f); }
    }
  }
  pthread_mutex_unlock(&ioTrava);
}

// DUAS ETAPAS EM FIOS SEPARADOS: o TMDB responde em ~0,15 s e ja da a resposta que o Play usa; o Wikidata
// (que so corrige 0,3-0,7% e pode levar segundos) fica num fio proprio. Com um fio so, o titulo olhado agora
// esperava ~6 s atras do Wikidata de um titulo do fundo (tests/idiomaorig_cache).
typedef struct { IoPedido p; IdiomaOrig r; } IoWd;
static IoWd ioWd[IO_FUNDO];
static int ioWdIni, ioWdN;
static pthread_cond_t ioWdSinal = PTHREAD_COND_INITIALIZER;

static void *ioFioTmdb(void *u) {
  (void)u;
  for (;;) {
    IoPedido p;
    char chave[80];
    IdiomaOrig r;
    IoSlot *s;
    long tmdb;
    pthread_mutex_lock(&ioTrava);
    while (!ioUrgN && !ioFundoN) pthread_cond_wait(&ioSinal, &ioTrava);
    if (ioUrgN) p = ioUrg[--ioUrgN];   // o mais recente primeiro: e o que a pessoa olha agora
    else { p = ioFundo[ioFundoIni]; ioFundoIni = (ioFundoIni + 1) % IO_FUNDO; ioFundoN--; }
    s = ioSlot(p.id);
    if (s && s->estado >= 2) { pthread_mutex_unlock(&ioTrava); continue; }   // repetido nas duas filas
    snprintf(chave, sizeof chave, "%s", ioChaveTmdb);
    pthread_mutex_unlock(&ioTrava);

    memset(&r, 0, sizeof r);
    tmdb = !strncmp(p.id, "tmdb:", 5) ? atol(p.id + 5) : 0;
    if (chave[0]) {
      if (tmdb) tmdbOriginal(tmdb, p.serie, chave, &r);
      else tmdbFind(p.id, p.serie, chave, &r);
      if (r.lang[IO_TMDB_FIND][0]) ioGuardar(p.id, r.lang[IO_TMDB_FIND], 0);
    }
    pthread_mutex_lock(&ioTrava);
    if (ioWdN < IO_FUNDO) {
      int q = (ioWdIni + ioWdN++) % IO_FUNDO;
      ioWd[q].p = p; ioWd[q].r = r;
      pthread_cond_signal(&ioWdSinal);
    } else if ((s = ioSlot(p.id)) != NULL && s->estado < 2) s->estado = 0;   // sem vaga: consulta de novo depois
    pthread_mutex_unlock(&ioTrava);
  }
  return NULL;
}

static void *ioFioWd(void *u) {
  (void)u;
  for (;;) {
    IoWd w;
    long tmdb;
    char chave[80];
    pthread_mutex_lock(&ioTrava);
    while (!ioWdN) pthread_cond_wait(&ioWdSinal, &ioTrava);
    w = ioWd[ioWdIni]; ioWdIni = (ioWdIni + 1) % IO_FUNDO; ioWdN--;
    pthread_mutex_unlock(&ioTrava);
    tmdb = !strncmp(w.p.id, "tmdb:", 5) ? atol(w.p.id + 5) : 0;
    pthread_mutex_lock(&ioTrava); snprintf(chave, sizeof chave, "%s", ioChaveTmdb); pthread_mutex_unlock(&ioTrava);
    if (chave[0]) tmdbDetalhes(w.p.serie, chave, &w.r);   // so a decisao precisa do idioma falado
    if (tmdb) wikidataTmdb(tmdb, w.p.serie, &w.r);
    else wikidata(w.p.id, &w.r);
    // Os 2 erros que sobraram nas amostras eram o Wikidata estourando o prazo: uma segunda tentativa.
    if (!w.r.wdTodas[0] && w.r.ms[IO_WIKIDATA] >= IO_PRAZO_S * 1000 - 100) {
      if (tmdb) wikidataTmdb(tmdb, w.p.serie, &w.r); else wikidata(w.p.id, &w.r);
    }
    if (!tmdb && !w.r.lang[IO_TMDB_FIND][0] && !w.r.lang[IO_WIKIDATA][0]) cinemeta(w.p.id, w.p.serie, &w.r);
    idiomaorig_decidir(&w.r);
    printf("[idioma-orig] %s: %s (tmdb=%s %dms, wikidata=%s %dms)\n", w.p.id, w.r.final[0] ? w.r.final : "?",
           w.r.lang[IO_TMDB_FIND][0] ? w.r.lang[IO_TMDB_FIND] : "-", w.r.ms[IO_TMDB_FIND],
           w.r.wdTodas[0] ? w.r.wdTodas : "-", w.r.ms[IO_WIKIDATA]);
    ioGuardar(w.p.id, w.r.final, 1);
  }
  return NULL;
}

void idiomaorig_iniciar(const char *arquivo) {
  FILE *f;
  char l[64], id[32], lang[8];
  int n = 0;
  pthread_mutex_lock(&ioTrava);
  snprintf(ioArquivo, sizeof ioArquivo, "%s", arquivo ? arquivo : "");
  if (ioArquivo[0] && (f = fopen(ioArquivo, "r")) != NULL) {
    while (fgets(l, sizeof l, f))
      if (sscanf(l, "%31[^\t]\t%7s", id, lang) == 2 && (imdbValido(id) || !strncmp(id, "tmdb:", 5))) {
        IoSlot *s = ioSlot(id);
        if (s) { snprintf(s->lang, sizeof s->lang, "%s", lang); s->estado = 3; n++; }
      }
    fclose(f);
  }
  pthread_mutex_unlock(&ioTrava);
  printf("[idioma-orig] %d resposta(s) gravada(s) em %s\n", n, ioArquivo[0] ? ioArquivo : "(memoria)");
}

// Com a trava: poe na fila se ainda nao se sabe; urgente tambem passa na frente de quem ja esta no fundo.
static void ioPedir(const char *id, int serie, int urgente) {
  IoSlot *s = ioSlot(id);
  if (!s || s->estado >= 2 || (s->estado == 1 && !urgente)) return;
  if (urgente) {
    if (ioUrgN == IO_URG) {   // sai o mais velho; volta a ser pedido da proxima vez que for olhado
      IoSlot *v = ioSlot(ioUrg[0].id);
      if (v && v->estado == 1) v->estado = 0;
      memmove(ioUrg, ioUrg + 1, sizeof ioUrg[0] * --ioUrgN);
    }
    snprintf(ioUrg[ioUrgN].id, sizeof ioUrg[0].id, "%s", id);
    ioUrg[ioUrgN++].serie = serie;
  } else {
    int q;
    if (ioFundoN == IO_FUNDO) return;   // cheia: entra quando for olhado (urgente) ou no proximo lote
    q = (ioFundoIni + ioFundoN++) % IO_FUNDO;
    snprintf(ioFundo[q].id, sizeof ioFundo[0].id, "%s", id);
    ioFundo[q].serie = serie;
  }
  s->estado = 1;
  while (ioFios < IO_FIOS) {   // um fio do TMDB e um do Wikidata
    pthread_t t;
    if (pthread_create(&t, NULL, ioFios ? ioFioWd : ioFioTmdb, NULL)) break;
    pthread_detach(t); ioFios++;
  }
  pthread_cond_signal(&ioSinal);
}

void idiomaorig_pedir(const char *imdb, long tmdb, int serie, const char *chaveTmdb) {
  char id[24];
  if (!ioChave(imdb, tmdb, id)) return;
  pthread_mutex_lock(&ioTrava);
  if (chaveTmdb && chaveTmdb[0]) snprintf(ioChaveTmdb, sizeof ioChaveTmdb, "%s", chaveTmdb);
  ioPedir(id, serie, 0);
  pthread_mutex_unlock(&ioTrava);
}

int idiomaorig_de(const char *imdb, int serie, const char *chaveTmdb, char out[8]) {
  char id[24];
  IoSlot *s;
  int final = 0;
  out[0] = 0;
  if (!ioChave(imdb, 0, id)) return 0;
  pthread_mutex_lock(&ioTrava);
  if (chaveTmdb && chaveTmdb[0]) snprintf(ioChaveTmdb, sizeof ioChaveTmdb, "%s", chaveTmdb);
  if ((s = ioSlot(id)) != NULL) {
    if (s->estado >= 2) { snprintf(out, 8, "%s", s->lang); final = s->estado == 3; }
    else ioPedir(id, serie, 1);
  }
  pthread_mutex_unlock(&ioTrava);
  return final;
}
