// AUDIO ORIGINAL ANTES DE QUALIDADE (dono, 2026-10-27), no streams.c de verdade: com "Áudio: Original" e
// um filme coreano, o automatico percorre a lista de cima para baixo e toca a primeira fonte que DIZ ter
// audio coreano; sem nenhuma etiquetada, a melhor da lista (a de sempre).
//   bash tests/fonte_audio_original.sh
#include "../src/streams.c"
#include <assert.h>

static Stream S[4];
static void fonte(int i, const char *rotulo, int altura) {
  memset(&S[i], 0, sizeof S[i]);
  snprintf(S[i].rotulo, sizeof S[i].rotulo, "%s", rotulo);
  snprintf(S[i].url, sizeof S[i].url, "https://exemplo.invalid/%d.mkv", i);
  S[i].altura = altura;
}

int main(void) {
  lista = S; n = 4; listaGeracao++;
  ling_local_audio("~"); ling_definir_original("ko");
  fonte(0, "Filme 2160p WEB-DL", 2160);              // a melhor, sem etiqueta de audio
  fonte(1, "Filme 1080p BluRay ENG", 1080);          // dublado em ingles
  fonte(2, "Filme 1080p WEB-DL KOR", 1080);          // a primeira com coreano
  fonte(3, "Filme 720p Korean", 720);
  assert(stream_automatico() == 2 && "volta 1: a primeira que diz coreano");
  fonte(2, "Filme 1080p WEB-DL MULTI", 1080);        // multi nao garante: pula
  listaGeracao++;
  assert(stream_automatico() == 3 && "multi nao conta; a 720p diz coreano");
  fonte(3, "Filme 720p", 720);
  listaGeracao++;
  assert(stream_automatico() == 0 && "volta 2: nenhuma diz, a melhor da lista");
  S[0].audioIdiomas[0] = 0; snprintf(S[1].audioIdiomas, sizeof S[1].audioIdiomas, "Korean, English");
  listaGeracao++;
  assert(stream_automatico() == 1 && "a lista de audio do addon (Korean) tambem conta");
  ling_local_audio("*");                              // "Todas": sem regra de idioma
  listaGeracao++;
  assert(stream_automatico() == 0);
  puts("fonte_audio_original: ok");
  return 0;
}
