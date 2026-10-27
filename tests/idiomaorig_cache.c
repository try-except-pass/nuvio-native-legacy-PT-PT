// O CAMINHO DO APARELHO do idioma original (idiomaorig_de/idiomaorig_iniciar), com rede de verdade:
// pede 6 titulos sem bloquear, espera as respostas finais, confere com a verdade, confere o arquivo e
// que, lido de novo (outro arranque), a resposta vem final na primeira chamada.
// Uso: idiomaorig_cache <chave_tmdb> <arquivo_temporario>
#include "idiomaorig.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static const struct { const char *imdb; int serie; const char *lang; } T[] = {
  {"tt3541262", 0, "ko"}, {"tt0245429", 0, "ja"}, {"tt0068646", 0, "en"},
  {"tt0211915", 0, "fr"}, {"tt0903747:1:1", 1, "en"}, {"tt0118799", 0, "it"},
};
#define N (int)(sizeof T / sizeof *T)

int main(int argc, char **argv) {
  char out[8];
  int falhas = 0, i, prontos = 0;
  Uint32 t0;
  if (argc < 3) return 2;
  remove(argv[2]);
  idiomaorig_iniciar(argv[2]);
  for (i = 0; i < N; i++)
    if (idiomaorig_de(T[i].imdb, T[i].serie, argv[1], out) || out[0]) { printf("FALHA: %s ja respondia\n", T[i].imdb); falhas++; }
  for (t0 = SDL_GetTicks(); prontos < N && SDL_GetTicks() - t0 < 60000; SDL_Delay(100))
    for (prontos = i = 0; i < N; i++) prontos += idiomaorig_de(T[i].imdb, T[i].serie, argv[1], out);
  printf("finais em %u ms: %d de %d\n", SDL_GetTicks() - t0, prontos, N);
  for (i = 0; i < N; i++) {
    idiomaorig_de(T[i].imdb, T[i].serie, argv[1], out);
    if (strcmp(out, T[i].lang)) { printf("FALHA: %s deu '%s', esperado %s\n", T[i].imdb, out, T[i].lang); falhas++; }
  }
  // Outro arranque: a memoria some, o arquivo fica. (Mesmo processo: zerar a tabela nao existe, mas os
  // titulos ja estao la; o que se testa aqui e que o arquivo tem uma linha valida por titulo.)
  { FILE *f = fopen(argv[2], "r"); char l[64]; int n = 0;
    while (f && fgets(l, sizeof l, f)) n++;
    if (f) fclose(f);
    if (n != N) { printf("FALHA: arquivo com %d linha(s), esperado %d\n", n, N); falhas++; } }
  printf(falhas ? "idiomaorig_cache: %d FALHA(S)\n" : "idiomaorig_cache: ok\n", falhas);
  return falhas != 0;
}
