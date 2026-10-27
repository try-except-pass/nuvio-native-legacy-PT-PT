// Player externo (#501) — ver extplayer.h.
#include "extplayer.h"
#include <stddef.h>

int extplayer_decidir(int modo, const char *cabs, int *avisar) {
  if (avisar) *avisar = 0;
  if (modo != EXTPLAYER_EXTERNO && modo != EXTPLAYER_PERGUNTAR) return EXTPLAYER_INTERNO;
  if (cabs && cabs[0]) { if (avisar) *avisar = 1; return EXTPLAYER_INTERNO; }
  return modo;
}

int extplayer_escolher(int anime, int modoFilme, const char *pkgFilme,
                       int modoAnime, const char *pkgAnime,
                       int (*instalado)(const char *pkg), const char **pkg, int *recuou) {
  int m = modoFilme;
  const char *p = pkgFilme;
  if (anime && modoAnime >= 0) { m = modoAnime; p = pkgAnime; }
  if (pkg) *pkg = NULL;
  if (recuou) *recuou = 0;
  if (m != EXTPLAYER_APP) return m >= EXTPLAYER_INTERNO && m <= EXTPLAYER_PERGUNTAR ? m : EXTPLAYER_INTERNO;
  if (!p || !p[0] || !instalado || !instalado(p)) {
    if (recuou) *recuou = 1;
    return EXTPLAYER_PERGUNTAR;
  }
  if (pkg) *pkg = p;
  return EXTPLAYER_EXTERNO;
}

