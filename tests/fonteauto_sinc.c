// A ESCOLHA QUE MIRA UMA LEGENDA SINCRONIZADA (dono, 2026-10-27; HANDOFF §4 item 2): fonteauto_sincronizada.
// Sem rede. Cobre a ordem do dono, os passos pulados (sem secundaria / sem "Original"), que SEM nenhuma
// informacao de sincronia o resultado e o de sempre (a 1a da fila), e que o custo e desprezivel ao lado da fila.
//
//   bash tests/fonteauto_sinc.sh
#include "fonteauto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int falhas;
#define CONFERE(c, ...) do { if (!(c)) { falhas++; printf("FALHOU %s:%d: ", __FILE__, __LINE__); \
  printf(__VA_ARGS__); printf("\n"); } } while (0)

#define N 30
static FonteSinc f[N];
static int fila[N];

static void limpa(int n) {
  memset(f, 0, sizeof f);
  for (int i = 0; i < n; i++) fila[i] = i;
}
static int escolhe(int n, int exigeOriginal, int temSec, int *nivel) {
  return fonteauto_sincronizada(fila, n, f, exigeOriginal, temSec, nivel);
}

// O ORACULO: escrito de outro jeito (uma chave por fonte, vence a menor), para nao repetir o laco da regra.
static int oraculo(int n, int exigeOriginal, int temSec, int *nivel) {
  int soOrig = 0, melhor = -1, chave, q, melhorChave = 1 << 30;
  if (exigeOriginal) for (q = 0; q < n; q++) soOrig |= f[fila[q]].original;
  for (q = 0; q < n; q++) {
    const FonteSinc *s = &f[fila[q]];
    int nv = 9;
    if (soOrig && !s->original) continue;
    if (s->sincPrim == FSINC_CONFIRMADA) nv = 1;
    else if (temSec && s->sincSec == FSINC_CONFIRMADA) nv = 2;
    else if (s->temPrim) nv = 3;
    else if (temSec && s->temSec) nv = 4;
    // um nivel melhor numa fonte mais ABAIXO ganha de um nivel pior mais acima: o nivel vem antes da posicao
    chave = nv * 1000 + q;
    if (chave < melhorChave) { melhorChave = chave; melhor = fila[q]; }
  }
  if (melhor < 0 || melhorChave >= 9000) {   // ninguem tem legenda: a 1a elegivel
    *nivel = FSINC_NADA;
    for (q = 0; q < n; q++) if (!soOrig || f[fila[q]].original) return fila[q];
    return fila[0];
  }
  *nivel = melhorChave / 1000;
  return melhor;
}

static double agora(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1e9 + t.tv_nsec;
}

int main(void) {
  int nv, r, i, n;

  // sem fila / sem dados
  CONFERE(fonteauto_sincronizada(NULL, 3, f, 1, 1, &nv) == -1 && nv == FSINC_NADA, "sem fila");
  limpa(0);
  CONFERE(escolhe(0, 1, 1, &nv) == -1, "fila vazia");

  // a sincronizada mais abaixo ganha da 1a sem sincronia confirmada
  limpa(6);
  for (i = 0; i < 6; i++) f[i].temPrim = 1;
  f[3].sincPrim = FSINC_CONFIRMADA;
  r = escolhe(6, 0, 1, &nv);
  CONFERE(r == 3 && nv == FSINC_PRIM_OK, "principal sincronizada: %d nivel %d", r, nv);

  // duas sincronizadas: vence a que vem antes na fila
  f[2].sincPrim = FSINC_CONFIRMADA;
  CONFERE(escolhe(6, 0, 1, &nv) == 2, "empate: a de cima");

  // a ordem da fila e respeitada (fila embaralhada)
  fila[0] = 5; fila[1] = 3; fila[2] = 2; fila[3] = 0; fila[4] = 1; fila[5] = 4;
  CONFERE(escolhe(6, 0, 1, &nv) == 3, "ordem da fila: 3 vem antes de 2");

  // audio original vem ANTES da sincronia: a original sem sincronia ganha da nao original sincronizada
  limpa(6);
  for (i = 0; i < 6; i++) f[i].temPrim = 1;
  f[5].original = 1;
  f[1].sincPrim = FSINC_CONFIRMADA;
  r = escolhe(6, 1, 1, &nv);
  CONFERE(r == 5 && nv == FSINC_PRIM, "original antes da sincronia: %d nivel %d", r, nv);
  // sem "Original" ligado a marca nao separa ninguem
  r = escolhe(6, 0, 1, &nv);
  CONFERE(r == 1 && nv == FSINC_PRIM_OK, "sem Original: %d nivel %d", r, nv);
  // "Original" ligado mas nenhuma fonte diz ter o original: todas disputam
  f[5].original = 0;
  r = escolhe(6, 1, 1, &nv);
  CONFERE(r == 1 && nv == FSINC_PRIM_OK, "ninguem diz ter o original: %d nivel %d", r, nv);

  // secundaria sincronizada ganha da principal NAO confirmada
  limpa(6);
  for (i = 0; i < 6; i++) f[i].temPrim = 1;
  f[4].sincSec = FSINC_CONFIRMADA;
  r = escolhe(6, 0, 1, &nv);
  CONFERE(r == 4 && nv == FSINC_SEC_OK, "secundaria sincronizada: %d nivel %d", r, nv);
  // ... mas SEM secundaria configurada o passo e pulado
  r = escolhe(6, 0, 0, &nv);
  CONFERE(r == 0 && nv == FSINC_PRIM, "sem secundaria: %d nivel %d", r, nv);
  // a principal sincronizada ainda ganha da secundaria sincronizada
  f[2].sincPrim = FSINC_CONFIRMADA;
  CONFERE(escolhe(6, 0, 1, &nv) == 2 && nv == FSINC_PRIM_OK, "principal ganha da secundaria");

  // conferida e NAO casa: continua valendo como "principal sem confirmacao" (o dono quer a PT-PT mesmo assim)
  limpa(4);
  f[2].temPrim = 1; f[2].sincPrim = FSINC_NAO;
  r = escolhe(4, 0, 1, &nv);
  CONFERE(r == 2 && nv == FSINC_PRIM, "nao casa mas existe: %d nivel %d", r, nv);

  // so a secundaria nao confirmada
  limpa(4);
  f[3].temSec = 1;
  r = escolhe(4, 0, 1, &nv);
  CONFERE(r == 3 && nv == FSINC_SEC, "so a secundaria: %d nivel %d", r, nv);
  CONFERE(escolhe(4, 0, 0, &nv) == 0 && nv == FSINC_NADA, "sem secundaria configurada, nada");

  // ninguem tem legenda: a 1a da fila; com Original, a 1a que diz ter o original
  limpa(5);
  CONFERE(escolhe(5, 1, 1, &nv) == 0 && nv == FSINC_NADA, "nada: 1a");
  f[3].original = 1; f[4].original = 1;
  CONFERE(escolhe(5, 1, 1, &nv) == 3 && nv == FSINC_NADA, "nada, com original: a 1a original");

  // PROPRIEDADE 1: contra o oraculo, em casos aleatorios
  srand(20261027);
  for (int caso = 0; caso < 20000; caso++) {
    int e, ts, ro, no;
    n = 1 + rand() % N;
    limpa(n);
    for (i = 0; i < n; i++) {
      f[i].original = rand() % 3 == 0;
      f[i].temPrim = rand() % 2;
      f[i].temSec = rand() % 2;
      f[i].sincPrim = (unsigned char)(rand() % 6 == 0 ? FSINC_CONFIRMADA : rand() % 4 == 0 ? FSINC_NAO : 0);
      f[i].sincSec = (unsigned char)(rand() % 6 == 0 ? FSINC_CONFIRMADA : rand() % 4 == 0 ? FSINC_NAO : 0);
    }
    for (i = n - 1; i > 0; i--) { int j = rand() % (i + 1), t = fila[i]; fila[i] = fila[j]; fila[j] = t; }
    e = rand() % 2; ts = rand() % 2;
    r = escolhe(n, e, ts, &nv);
    ro = oraculo(n, e, ts, &no);
    CONFERE(r == ro && nv == no, "caso %d: regra %d/%d, oraculo %d/%d", caso, r, nv, ro, no);
    if (falhas > 5) break;
  }

  // PROPRIEDADE 2: SEM nenhuma informacao de sincronia, o resultado e o de sempre. A fila de hoje e
  // fonteauto_idioma_primeiro; a regra nova tem de devolver a 1a dela, nem uma outra.
  for (int caso = 0; caso < 20000 && falhas < 5; caso++) {
    unsigned char tem[N];
    n = 1 + rand() % N;
    limpa(n);
    for (i = 0; i < n; i++) { f[i].original = rand() % 3 == 0; f[i].temPrim = 1; tem[i] = f[i].original; }
    fonteauto_idioma_primeiro(fila, n, tem);
    r = escolhe(n, 1, rand() % 2, &nv);
    CONFERE(r == fila[0], "sem sincronia: devolveu %d, a de sempre e %d", r, fila[0]);
  }

  // CUSTO: o mesmo tamanho do dataset (57 titulos x 30 fontes). A fila de hoje e quadratica; a regra nova e linear.
  {
    enum { TIT = 57, VOLTAS = 2000 };
    static FonteSinc fs[TIT][N];
    static unsigned char tem[TIT][N];
    static long pontos[TIT][N];
    int sorvedouro = 0, tmp[N];
    double t0, antes, depois;
    for (int t = 0; t < TIT; t++)
      for (i = 0; i < N; i++) {
        fs[t][i].original = tem[t][i] = rand() % 3 == 0;
        fs[t][i].temPrim = 1;
        fs[t][i].sincPrim = (unsigned char)(rand() % 8 == 0 ? FSINC_CONFIRMADA : 0);
        pontos[t][i] = (rand() % 6) * 10000L;
      }
    t0 = agora();
    for (int v = 0; v < VOLTAS; v++)
      for (int t = 0; t < TIT; t++) {
        n = fonteauto_fila(FONTEAUTO_MELHOR, N, -1, pontos[t], NULL, NULL, N, tmp);
        fonteauto_idioma_primeiro(tmp, n, tem[t]);
        sorvedouro += tmp[0];
      }
    antes = (agora() - t0) / (VOLTAS * TIT);
    t0 = agora();
    for (int v = 0; v < VOLTAS; v++)
      for (int t = 0; t < TIT; t++) {
        n = fonteauto_fila(FONTEAUTO_MELHOR, N, -1, pontos[t], NULL, NULL, N, tmp);
        fonteauto_idioma_primeiro(tmp, n, tem[t]);
        sorvedouro += fonteauto_sincronizada(tmp, n, fs[t], 1, 1, &nv);
      }
    depois = (agora() - t0) / (VOLTAS * TIT);
    printf("custo por escolha (30 fontes): fila de hoje %.0f ns; com a regra nova %.0f ns (+%.0f ns)  [%d]\n",
           antes, depois, depois - antes, sorvedouro & 1);
    CONFERE(depois - antes < antes, "a regra nova custa mais que a fila inteira: +%.0f ns contra %.0f ns", depois - antes, antes);
  }

  if (falhas) { printf("fonteauto_sinc: %d falha(s)\n", falhas); return 1; }
  printf("fonteauto_sinc: ok\n");
  return 0;
}
