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

// NO APARELHO. Le as respostas gravadas (uma linha "tt123<TAB>ko" por titulo) e passa a gravar as novas
// em `arquivo` ("" ou NULL = so na memoria). Chamar uma vez no arranque.
void idiomaorig_iniciar(const char *arquivo);
// Onde esta a busca de um titulo. Todo titulo tem um (dono, 2026-10-27): so depois de ACHADO ou SEM a fonte
// e escolhida.
enum { IDIOMA_NUNCA = 0, IDIOMA_FILA, IDIOMA_CORRENDO, IDIOMA_ACHADO, IDIOMA_SEM };
// Nao bloqueia: copia em `out` o idioma que se sabe agora ("" = nada ainda; CORRENDO pode trazer a do TMDB,
// provisoria; SEM traz "en") e, se ainda nao foi feito, poe o titulo na fila URGENTE. Devolve o estado.
int idiomaorig_de(const char *imdb, int serie, const char *chaveTmdb, char out[8]);
// Bloqueia (fio proprio) ate ACHADO/SEM, `prazoMs` ou `seguir(u)` == 0. Sem resposta final, `out` fica com a
// provisoria do TMDB ou "en". Devolve o estado.
int idiomaorig_esperar(const char *imdb, int serie, const char *chaveTmdb, char out[8], int prazoMs,
                       int (*seguir)(void *), void *u);
// Para TODO titulo que entra no catalogo (sem bloquear, fila de fundo; o que ja se sabe nao vai). `imdb` pode
// ser "tt..." ou "tmdb:N"; sem IMDb usa `tmdb` (> 0) se houver.
void idiomaorig_pedir(const char *imdb, long tmdb, int serie, const char *chaveTmdb);

#endif
