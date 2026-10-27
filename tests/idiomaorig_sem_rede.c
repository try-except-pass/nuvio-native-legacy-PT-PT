// REDE FORA nao e "tentado": com todas as fontes sem resposta, o titulo nao fica SEM (ingles para sempre na
// sessao); vale "en" ate la e volta a ser pedido (dono, 2026-10-27: o audio original nao pode falhar).
#include "idiomaorig.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int pedidos;
char *rede_baixar(const char *url, int segundos) { (void)url; (void)segundos; pedidos++; return NULL; }

int main(void) {
  char out[8];
  int e;
  SDL_Init(SDL_INIT_TIMER);
  idiomaorig_iniciar("");
  e = idiomaorig_esperar("tt5311514", 0, "chave", out, 1500, NULL, NULL);
  assert(e < IDIOMA_ACHADO && !strcmp(out, "en"));
  SDL_Delay(200);
  e = idiomaorig_de("tt5311514", 0, "chave", out);
  assert(e != IDIOMA_SEM && !strcmp(out, "en"));
  { int antes = pedidos; SDL_Delay(2600); idiomaorig_de("tt5311514", 0, "chave", out); SDL_Delay(300);
    assert(pedidos > antes); }   // pedido de novo depois da espera, nao esquecido
  assert(pedidos < 40);          // e sem martelar a rede: uma volta a cada 2 s
  printf("idiomaorig_sem_rede: ok (%d pedidos)\n", pedidos);
  return 0;
}
