// O esquema que a pagina de ajustes recebe (ajustes_web_esquema) e a gravacao
// (ajustes_web_aplicar) contra a tabela REAL de Ajustes, sem tela.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "ajustes.h"
#include "dados.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"

int main(int argc, char **argv) {
  const char *dir = getenv("NUVIO_DADOS");
  char *j, *p;
  int n = 0;
  (void)argc; (void)argv;
  if (!dir || !*dir) return 2;
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  dados_iniciar(dir);
  ajustes_iniciar();
  ajustes_dir(dir);   // sem a pasta gravar() recusa, como no app antes do login
  j = ajustes_web_esquema();
  assert(j && j[0] == '{');
  for (p = j; (p = strstr(p, "\"k\":")) != NULL; p++) n++;
  printf("esquema: %zu bytes, %d ajustes\n", strlen(j), n);
  FILE *f = fopen(argc > 1 ? argv[1] : "/tmp/ajweb-esquema.json", "w");
  if (f) { fputs(j, f); fclose(f); }
  assert(n > 150);
  assert(strstr(j, "\"sombraFocoLocal\""));
  assert(!strstr(j, "\"ajustesWebLocal\""));            // o proprio interruptor nao aparece
  free(j);
  assert(ajustes_web_aplicar("sombraFocoLocal", 1));      // desliga a sombra
  assert(!ajustes_sombra_foco());
  assert(ajustes_web_aplicar("sombraFocoLocal", 0));
  assert(ajustes_sombra_foco());
  assert(!ajustes_web_aplicar("naoExiste", 0));
  assert(!ajustes_web_aplicar("ajustesWebLocal", 0));    // nem pela porta dos fundos
  puts("ajweb_esquema: ok");
  return 0;
}
