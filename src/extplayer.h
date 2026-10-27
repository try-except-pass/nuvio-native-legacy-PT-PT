// Player externo (#501): Interno / Externo / Perguntar sempre. Ver extplayer.c.
#ifndef NV_EXTPLAYER_H
#define NV_EXTPLAYER_H

enum { EXTPLAYER_INTERNO = 0, EXTPLAYER_EXTERNO = 1, EXTPLAYER_PERGUNTAR = 2, EXTPLAYER_APP = 3 };
#define EXTPLAYER_APPS_MAX 12

// Qual modo vale para este titulo. Anime usa `modoAnime`; < 0 = igual ao Player.
// EXTPLAYER_APP (um app instalado, `pkg*`) sem o app instalado vira Perguntar
// (`*recuou` = 1, quem chama registra). Com o app, devolve EXTPLAYER_EXTERNO e
// `*pkg` = pacote; nos outros modos `*pkg` = NULL.
int extplayer_escolher(int anime, int modoFilme, const char *pkgFilme,
                       int modoAnime, const char *pkgAnime,
                       int (*instalado)(const char *pkg), const char **pkg, int *recuou);

// `modo` e o valor do ajuste (0 interno, 1 externo, 2 perguntar); `cabs` os
// cabecalhos do stream ("" = nenhum). Cabecalho nao cabe numa Intent: com ele
// o externo cai no interno e `*avisar` = 1 (quem chama avisa o usuario).
int extplayer_decidir(int modo, const char *cabs, int *avisar);

#endif
