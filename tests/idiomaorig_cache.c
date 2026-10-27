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
  SDL_Init(SDL_INIT_TIMER);   // o app sempre inicia; sem isto o relogio dos fios pode voltar para tras
  remove(argv[2]);
  idiomaorig_iniciar(argv[2]);
  for (i = 0; i < N; i++)
    if (idiomaorig_de(T[i].imdb, T[i].serie, argv[1], out) >= IDIOMA_ACHADO || out[0]) { printf("FALHA: %s ja respondia\n", T[i].imdb); falhas++; }
  for (t0 = SDL_GetTicks(); prontos < N && SDL_GetTicks() - t0 < 60000; SDL_Delay(100))
    for (prontos = i = 0; i < N; i++) prontos += idiomaorig_de(T[i].imdb, T[i].serie, argv[1], out) >= IDIOMA_ACHADO;
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
    if (n < N) { printf("FALHA: arquivo com %d linha(s), esperado %d\n", n, N); falhas++; } }
  // CATALOGO INTEIRO: 40 titulos na fila de fundo (como um lote da Home) e, com ela cheia, um titulo
  // olhado agora (urgente) tem de responder antes de o fundo acabar. Mais um so com id do TMDB.
  { static const char *LOTE[] = {"tt0110413","tt0114369","tt0102926","tt0120737","tt0167260","tt0133093",
      "tt0080684","tt0137523","tt0109830","tt0816692","tt0245429","tt0317248","tt0118799","tt0110912",
      "tt0099685","tt0073486","tt0047478","tt0038650","tt0076759","tt0103064","tt0088763","tt0095765",
      "tt0082971","tt0172495","tt0407887","tt0209144","tt0078788","tt0482571","tt0078748","tt0032553",
      "tt0095327","tt0043014","tt0057012","tt0050083","tt0405094","tt0081505","tt0050825","tt0910970",
      "tt0119698","tt0364569"};
    int nl = (int)(sizeof LOTE / sizeof *LOTE), prontosL = 0, urgOk = 0;
    Uint32 tUrg = 0, tFim = 0;
    for (i = 0; i < nl; i++) idiomaorig_pedir(LOTE[i], 0, 0, argv[1]);
    idiomaorig_pedir("tmdb:t1399", 1399, 1, argv[1]);   // Game of Thrones, so pelo id do TMDB
    t0 = SDL_GetTicks();
    idiomaorig_de("tt6751668", 0, argv[1], out);        // Parasite, olhado agora
    for (; SDL_GetTicks() - t0 < 120000; SDL_Delay(50)) {
      // O Play espera a resposta FINAL (dono, 2026-10-27), e o fundo nao a pode atrasar: fio urgente proprio.
      if (!urgOk && idiomaorig_de("tt6751668", 0, argv[1], out) >= IDIOMA_ACHADO) { urgOk = 1; tUrg = SDL_GetTicks() - t0; }
      if (!urgOk) continue;   // idiomaorig_de poe na fila urgente: so olhar o lote depois
      for (prontosL = i = 0; i < nl; i++) prontosL += idiomaorig_de(LOTE[i], 0, argv[1], out) >= IDIOMA_ACHADO;
      if (prontosL == nl && urgOk) { tFim = SDL_GetTicks() - t0; break; }
    }
    idiomaorig_de("tt6751668", 0, argv[1], out);
    printf("urgente (Parasite) em %u ms = %s; lote de %d pronto em %u ms (%d prontos)\n", tUrg, out, nl, tFim, prontosL);
    if (!urgOk || strcmp(out, "ko")) { printf("FALHA: urgente\n"); falhas++; }
    if (prontosL != nl) { printf("FALHA: lote incompleto\n"); falhas++; }
    if (tUrg > 5000) { printf("FALHA: o urgente demorou %u ms com o lote na fila\n", tUrg); falhas++; }
    for (t0 = SDL_GetTicks(); idiomaorig_de("tmdb:t1399", 1, argv[1], out) < IDIOMA_ACHADO && SDL_GetTicks() - t0 < 20000; SDL_Delay(100)) {}
    printf("tmdb:t1399 -> %s\n", out);
    if (strcmp(out, "en")) { printf("FALHA: id do TMDB\n"); falhas++; } }
  // PLAY COM O URGENTE OCUPADO: 30 titulos olhados agora (fila urgente cheia) e o Play de outro. A espera
  // do Play tem a cadeia num fio so dela: final bem antes do teto de 10 s.
  { static const char *OLHADOS[] = {"tt0021749","tt0022100","tt0025316","tt0027977","tt0031381","tt0032138",
      "tt0033467","tt0034583","tt0036775","tt0040522","tt0041959","tt0042192","tt0042876","tt0045152",
      "tt0046268","tt0046912","tt0047396","tt0048473","tt0050212","tt0050976","tt0051201","tt0052357",
      "tt0053125","tt0053291","tt0053604","tt0054215","tt0055031","tt0055630","tt0056172","tt0056592"};
    Uint32 t1;
    int e;
    for (i = 0; i < (int)(sizeof OLHADOS / sizeof *OLHADOS); i++) idiomaorig_de(OLHADOS[i], 0, argv[1], out);
    t1 = SDL_GetTicks();
    e = idiomaorig_esperar("tt5311514", 0, argv[1], out, 10000, NULL, NULL);   // Your Name
    printf("Play com o urgente ocupado: %s em %u ms (estado %d)\n", out, SDL_GetTicks() - t1, e);
    if (e != IDIOMA_ACHADO || strcmp(out, "ja") || SDL_GetTicks() - t1 > 5000) { printf("FALHA: Play\n"); falhas++; } }
  // Sem id nao ha busca: ja e "tentado", ingles.
  if (idiomaorig_de("", 0, argv[1], out) != IDIOMA_SEM || strcmp(out, "en")) { printf("FALHA: sem id deu '%s'\n", out); falhas++; }
  // Prazo estourado (algo quebrado): vale a provisoria ou "en", e o resto do app ve o mesmo.
  { int e = idiomaorig_esperar("tt0062622", 0, "", out, 1, NULL, NULL);   // sem chave: nada do TMDB
    char o2[8];
    if (e >= IDIOMA_ACHADO) printf("(tt0062622 ja final; teste do prazo nao vale)\n");
    else {
      idiomaorig_de("tt0062622", 0, "", o2);
      printf("prazo estourado -> '%s', depois '%s'\n", out, o2);
      if (!out[0] || strcmp(out, o2)) { printf("FALHA: prazo estourado\n"); falhas++; }
    } }
  printf(falhas ? "idiomaorig_cache: %d FALHA(S)\n" : "idiomaorig_cache: ok\n", falhas);
  return falhas != 0;
}
