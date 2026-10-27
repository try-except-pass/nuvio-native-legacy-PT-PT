// Idioma original de um titulo: ver idiomaorig.h.
#include "idiomaorig.h"
#include "js.h"
#include "rede.h"
#include <SDL2/SDL.h>
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
