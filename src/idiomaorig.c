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

// ---- No aparelho: tabela na memoria + arquivo + um fio que consulta um titulo por vez. ----
#define IO_SLOTS 4096   // potencia de 2; cheia, os titulos novos so ficam sem cache (consulta de novo)
#define IO_FILA 16
typedef struct { char id[16]; char lang[8]; unsigned char estado; } IoSlot;   // estado: 0 vazio, 1 na fila, 2 provisorio, 3 final
static IoSlot ioTab[IO_SLOTS];
static pthread_mutex_t ioTrava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ioSinal = PTHREAD_COND_INITIALIZER;
static struct { char id[16]; int serie; char chave[80]; } ioFila[IO_FILA];
static int ioFilaN, ioFioVivo;
static char ioArquivo[512];

static void ioChave(const char *imdb, char id[16]) {
  size_t n = strcspn(imdb, ":");
  if (n > 15) n = 15;
  memcpy(id, imdb, n); id[n] = 0;
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

static void *ioFio(void *u) {
  (void)u;
  for (;;) {
    char id[16], chave[80];
    int serie;
    IdiomaOrig r;
    IoSlot *s;
    pthread_mutex_lock(&ioTrava);
    while (!ioFilaN) pthread_cond_wait(&ioSinal, &ioTrava);
    ioFilaN--;
    snprintf(id, sizeof id, "%s", ioFila[ioFilaN].id);   // o mais recente primeiro: e o que a pessoa olha
    snprintf(chave, sizeof chave, "%s", ioFila[ioFilaN].chave);
    serie = ioFila[ioFilaN].serie;
    pthread_mutex_unlock(&ioTrava);

    memset(&r, 0, sizeof r);
    if (chave[0]) {
      tmdbFind(id, serie, chave, &r);
      if (r.lang[IO_TMDB_FIND][0]) {
        pthread_mutex_lock(&ioTrava);
        if ((s = ioSlot(id)) && s->estado < 3) { snprintf(s->lang, sizeof s->lang, "%s", r.lang[IO_TMDB_FIND]); s->estado = 2; }
        pthread_mutex_unlock(&ioTrava);
      }
      tmdbDetalhes(serie, chave, &r);
    }
    wikidata(id, &r);
    // Os 2 erros que sobraram nas amostras eram o Wikidata estourando o prazo: uma segunda tentativa.
    if (!r.wdTodas[0] && r.ms[IO_WIKIDATA] >= IO_PRAZO_S * 1000 - 100) wikidata(id, &r);
    if (!r.lang[IO_TMDB_FIND][0] && !r.lang[IO_WIKIDATA][0]) cinemeta(id, serie, &r);
    idiomaorig_decidir(&r);
    printf("[idioma-orig] %s: %s (tmdb=%s %dms, wikidata=%s %dms)\n", id, r.final[0] ? r.final : "?",
           r.lang[IO_TMDB_FIND][0] ? r.lang[IO_TMDB_FIND] : "-", r.ms[IO_TMDB_FIND],
           r.wdTodas[0] ? r.wdTodas : "-", r.ms[IO_WIKIDATA]);
    pthread_mutex_lock(&ioTrava);
    if ((s = ioSlot(id)) != NULL) {
      snprintf(s->lang, sizeof s->lang, "%s", r.final);
      // Sem resposta nenhuma (rede caida) nao vira final: a proxima vez consulta de novo.
      s->estado = r.final[0] ? 3 : 0;
      if (r.final[0] && ioArquivo[0]) {
        FILE *f = fopen(ioArquivo, "a");
        if (f) { fprintf(f, "%s\t%s\n", id, r.final); fclose(f); }
      }
    }
    pthread_mutex_unlock(&ioTrava);
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
      if (sscanf(l, "%31[^\t]\t%7s", id, lang) == 2 && imdbValido(id)) {
        IoSlot *s = ioSlot(id);
        if (s) { snprintf(s->lang, sizeof s->lang, "%s", lang); s->estado = 3; n++; }
      }
    fclose(f);
  }
  pthread_mutex_unlock(&ioTrava);
  printf("[idioma-orig] %d resposta(s) gravada(s) em %s\n", n, ioArquivo[0] ? ioArquivo : "(memoria)");
}

int idiomaorig_de(const char *imdb, int serie, const char *chaveTmdb, char out[8]) {
  char id[16];
  IoSlot *s;
  int final = 0;
  out[0] = 0;
  if (!imdb || !imdbValido(imdb)) return 0;
  ioChave(imdb, id);
  pthread_mutex_lock(&ioTrava);
  if ((s = ioSlot(id)) != NULL) {
    if (s->estado >= 2) { snprintf(out, 8, "%s", s->lang); final = s->estado == 3; }
    else if (s->estado == 0) {
      if (ioFilaN == IO_FILA) {   // cheia: sai o mais velho, que volta a ser pedido da proxima vez
        IoSlot *v = ioSlot(ioFila[0].id);
        if (v && v->estado == 1) v->estado = 0;
        memmove(ioFila, ioFila + 1, sizeof ioFila[0] * --ioFilaN);
      }
      snprintf(ioFila[ioFilaN].id, sizeof ioFila[0].id, "%s", id);
      snprintf(ioFila[ioFilaN].chave, sizeof ioFila[0].chave, "%s", chaveTmdb ? chaveTmdb : "");
      ioFila[ioFilaN++].serie = serie;
      s->estado = 1;
      if (!ioFioVivo) {
        pthread_t t;
        if (!pthread_create(&t, NULL, ioFio, NULL)) { pthread_detach(t); ioFioVivo = 1; }
      }
      pthread_cond_signal(&ioSinal);
    }
  }
  pthread_mutex_unlock(&ioTrava);
  return final;
}
