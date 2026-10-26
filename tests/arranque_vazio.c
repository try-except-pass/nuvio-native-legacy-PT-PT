// ARRANQUE COM O CATALOGO VAZIO (2.0.20: o cache antigo e recusado e a Home abre
// sem nenhum titulo ate a rede responder). O destaque montava a lista e lia o
// titulo de um item inexistente: crash em todo arranque. Aqui a Home desenha 300
// quadros com o catalogo zerado e uma tecla no meio; com AddressSanitizer
// (SANITIZE=1) qualquer leitura de ponteiro nulo derruba o teste.
#include "ajustes.h"
#include "badges.h"
#include "catalogo.h"
#include "gfx.h"
#include "home.h"
#include "menu.h"
#include "tex_cache.h"
#include "text.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int addons_n(void) { return 1; }

static void glIniciar(void) {
  enum { LW = 1920, LH = 1080 };
  GLuint fbo, tex;
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_Window *win = SDL_CreateWindow("vazio", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(win && SDL_GL_CreateContext(win));
  glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  badges_carregar("deploy/app/art");
}

int main(void) {
  const char *dados = getenv("NUVIO_DADOS");
  char caminho[700];
  FILE *f;
  Uint32 t = 100000;
  assert(dados && dados[0]);
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dados);
  f = fopen(caminho, "w"); assert(f); fputs("idioma 0\n", f); fclose(f);
  glIniciar();
  ajustes_dir(dados); ajustes_iniciar();
  assert(home_iniciar("deploy/app/art"));
  menu_iniciar();
  cat_definir_tudo(NULL, 0, NULL, 0);   // como no TV: sem cache, rede ainda sem resposta
  assert(cat_n() == 0);
  for (int i = 0; i < 300; i++) {
    t += 16;
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(8);
    glClear(GL_COLOR_BUFFER_BIT); gfx_novo_quadro();
    home_atualizar(1.f / 60, t); home_desenhar(t);
    menu_atualizar(1.f / 60, t); menu_desenhar(t);
    if (i == 100) {
      SDL_Event e = {0};
      e.type = SDL_KEYDOWN; e.key.keysym.sym = SDLK_RIGHT;
      home_evento(&e);
    }
  }
  printf("ok: 300 quadros da Home com o catalogo vazio\n");
  return 0;
}
