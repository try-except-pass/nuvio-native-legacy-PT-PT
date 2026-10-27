// IDIOMA ORIGINAL DE UM TITULO, ANTES DE ESCOLHER A FONTE (dono, 2026-10-26: "o audio original nao
// pode falhar"). Uma cadeia de fontes independentes, cada uma com prazo curto; a resposta e decidida
// por voto quando as fontes discordam e fica guardada no aparelho (a segunda vez e instantanea).
//
// Fontes (todas por HTTP, resposta JSON):
//   TMDB_FIND   /find/<imdb>              -> original_language (+ o id do TMDB)
//   WIKIDATA    SPARQL P345 (IMDb) -> P364 -> codigo ISO 639-1 (independente do TMDB)
//   TMDB_FALADO /movie|tv/<id>            -> spoken_languages[0], production_countries / origin_country
//   CINEMETA    /meta/<tipo>/<imdb>.json  -> "country" -> idioma principal do pais
// Bloqueia: chamar de um fio proprio, nunca do laco de desenho.
#ifndef NV_IDIOMAORIG_H
#define NV_IDIOMAORIG_H
#include <stddef.h>

enum { IO_TMDB_FIND = 0, IO_WIKIDATA, IO_TMDB_FALADO, IO_TMDB_PAIS, IO_CINEMETA_PAIS, IO_N };

typedef struct {
  char lang[IO_N][8];     // resposta de cada fonte ("" = nao respondeu / nao sabe)
  int  ms[IO_N];          // tempo de cada consulta
  char wdTodas[32];       // o Wikidata pode listar mais de um idioma original ("ko,en")
  long tmdbId;
  char final[8];          // a decisao da cadeia ("" = desconhecido)
  int  fonteFinal;        // qual regra decidiu (IO_*, ou -1 = voto)
} IdiomaOrig;

// Consulta TODAS as fontes (o teste usa isto para medir cada uma) e decide.
// `chaveTmdb` pode ser "" (as fontes TMDB ficam de fora).
void idiomaorig_consultar(const char *imdb, int serie, const char *chaveTmdb, IdiomaOrig *r);
// So a decisao, a partir das respostas ja preenchidas em `r` (testavel sem rede).
void idiomaorig_decidir(IdiomaOrig *r);
// Pais (nome em ingles ou ISO 3166-1 alfa-2) -> idioma principal; "" se o pais tem varios.
const char *idiomaorig_do_pais(const char *pais);

#endif
