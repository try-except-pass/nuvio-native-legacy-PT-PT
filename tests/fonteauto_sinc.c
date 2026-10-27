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

// A REDE FALSA da conferencia em serie: a verdade de cada (fonte, slot), quantas vezes cada uma foi perguntada.
static unsigned char verdade[N][2];
static int chamadas[N][2], total, cancelaApos = -1;
static int conferirFalso(int i, int slot, void *u) {
  (void)u;
  chamadas[i][slot]++;
  total++;
  return verdade[i][slot];
}
static int cancelaFalso(void *u) { (void)u; return cancelaApos >= 0 && total >= cancelaApos; }
static unsigned char emb[N];   // 1 = a fonte traz uma faixa de dialogo no idioma principal
static int perguntouEmb[N];
static int embFalso(int i, void *u) { (void)u; perguntouEmb[i]++; return emb[i]; }
// `tem`: toda fonte tem legenda principal (e a secundaria, com tem2); a verdade e o contador voltam a zero.
static void prepara(int n, int tem, int tem2) {
  limpa(n);
  memset(verdade, 0, sizeof verdade);
  memset(chamadas, 0, sizeof chamadas);
  total = 0;
  for (int i = 0; i < n; i++) { f[i].temPrim = (unsigned char)tem; f[i].temSec = (unsigned char)tem2; }
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

  // A CONFERENCIA EM SERIE (fonteauto_sincronizar), com a rede trocada por uma tabela de verdades.
  {
    FonteSincGancho g = { conferirFalso, cancelaFalso, NULL, NULL };
    int feitas;

    // para na primeira em que a principal sincroniza: 0 e 1 nao, 2 sim; 3..5 nem sao conferidas
    prepara(6, 1, 1);
    verdade[2][0] = FSINC_CONFIRMADA;
    r = fonteauto_sincronizar(fila, 6, f, 0, 1, &g, &nv, &feitas);
    CONFERE(r == 2 && nv == FSINC_PRIM_OK && feitas == 3, "para na 1a sincronizada: %d nivel %d, %d conferidas", r, nv, feitas);
    CONFERE(chamadas[3][0] + chamadas[4][0] + chamadas[5][0] == 0, "nao toca o que vem depois");

    // com Original ligado so as que dizem ter o original gastam leitura
    prepara(6, 1, 1);
    f[1].original = f[4].original = 1;
    verdade[4][0] = FSINC_CONFIRMADA;
    r = fonteauto_sincronizar(fila, 6, f, 1, 1, &g, &nv, &feitas);
    CONFERE(r == 4 && feitas == 2 && chamadas[0][0] + chamadas[2][0] + chamadas[3][0] + chamadas[5][0] == 0,
            "so as originais: %d, %d conferidas", r, feitas);
    // ... e sem nenhuma original, todas disputam
    prepara(6, 1, 1);
    verdade[5][0] = FSINC_CONFIRMADA;
    r = fonteauto_sincronizar(fila, 6, f, 1, 1, &g, &nv, &feitas);
    CONFERE(r == 5 && feitas == 6, "ninguem original: todas, %d conferidas", feitas);

    // a secundaria so e perguntada com temSec; e uma fonte sem legenda nao e perguntada de nada
    prepara(4, 1, 1);
    f[1].temPrim = 0;
    fonteauto_sincronizar(fila, 4, f, 0, 0, &g, &nv, &feitas);
    CONFERE(chamadas[0][1] + chamadas[1][1] + chamadas[2][1] + chamadas[3][1] == 0, "sem secundaria: slot 1 nunca");
    CONFERE(chamadas[1][0] == 0, "sem legenda principal: nao pergunta");
    prepara(4, 1, 1);
    fonteauto_sincronizar(fila, 4, f, 0, 1, &g, &nv, &feitas);
    CONFERE(chamadas[0][1] == 1 && chamadas[3][1] == 1, "com secundaria: slot 1 uma vez por fonte");

    // principal sincronizada MAIS ABAIXO ganha da secundaria sincronizada de cima: a conferencia vai ate la
    prepara(6, 1, 1);
    verdade[1][1] = FSINC_CONFIRMADA;
    verdade[4][0] = FSINC_CONFIRMADA;
    r = fonteauto_sincronizar(fila, 6, f, 0, 1, &g, &nv, &feitas);
    CONFERE(r == 4 && nv == FSINC_PRIM_OK && feitas == 5, "principal abaixo ganha da secundaria acima: %d/%d", r, nv);

    // ninguem sincroniza: confere todas e vale a regra dos niveis (principal sem confirmacao)
    prepara(5, 1, 1);
    r = fonteauto_sincronizar(fila, 5, f, 0, 1, &g, &nv, &feitas);
    CONFERE(r == 0 && nv == FSINC_PRIM && feitas == 5, "ninguem sincroniza: %d nivel %d, %d conferidas", r, nv, feitas);

    // cancelado: para e nao volta a perguntar
    prepara(6, 1, 1);
    cancelaApos = 2;
    r = fonteauto_sincronizar(fila, 6, f, 0, 0, &g, &nv, &feitas);
    CONFERE(feitas == 2 && total == 2, "cancelado depois de 2: %d conferidas, %d chamadas", feitas, total);
    cancelaApos = -1;

    // sem gancho / sem fila
    CONFERE(fonteauto_sincronizar(fila, 3, f, 0, 0, NULL, &nv, &feitas) == -1 && feitas == 0, "sem gancho");

    // FAIXA EMBUTIDA: a fonte 3 traz o idioma principal dentro dela e NENHUMA fonte tem legenda de addon: degrau 1
    {
      FonteSincGancho ge = { conferirFalso, cancelaFalso, NULL, embFalso };
      prepara(6, 0, 0);
      memset(emb, 0, sizeof emb); memset(perguntouEmb, 0, sizeof perguntouEmb);
      emb[3] = 1;
      r = fonteauto_sincronizar(fila, 6, f, 0, 1, &ge, &nv, &feitas);
      CONFERE(r == 3 && nv == FSINC_PRIM_OK && feitas == 4, "embutida sem legenda de addon: %d nivel %d, %d conferidas", r, nv, feitas);
      CONFERE(total == 0 && perguntouEmb[4] + perguntouEmb[5] == 0, "para na embutida: %d chamadas ao addon", total);
      // a de cima que sincroniza pelo addon ganha da embutida mais abaixo (a posicao decide no mesmo degrau)
      prepara(6, 1, 0); memset(emb, 0, sizeof emb); emb[4] = 1; verdade[2][0] = FSINC_CONFIRMADA;
      CONFERE(fonteauto_sincronizar(fila, 6, f, 0, 1, &ge, &nv, &feitas) == 2, "addon sincronizada acima da embutida");
      // e a embutida acima ganha da sincronizada pelo addon abaixo
      prepara(6, 1, 0); memset(emb, 0, sizeof emb); emb[1] = 1; verdade[3][0] = FSINC_CONFIRMADA;
      CONFERE(fonteauto_sincronizar(fila, 6, f, 0, 1, &ge, &nv, &feitas) == 1 && feitas == 2, "embutida acima da addon");
      // com Original ligado, a embutida de uma fonte que nao e original nao disputa
      prepara(6, 0, 0); memset(emb, 0, sizeof emb); memset(perguntouEmb, 0, sizeof perguntouEmb); emb[1] = 1; f[4].original = 1; f[5].original = 1;
      r = fonteauto_sincronizar(fila, 6, f, 1, 1, &ge, &nv, &feitas);
      CONFERE(r == 4 && nv == FSINC_NADA && perguntouEmb[0] == 0, "so as originais: %d nivel %d", r, nv);
    }

    // PROPRIEDADE 3: parar cedo NUNCA muda a resposta. Mesma verdade, uma vez com a conferencia em serie e outra
    // com a tabela toda preenchida de uma vez e so a regra por cima.
    for (int caso = 0; caso < 20000 && falhas < 5; caso++) {
      FonteSinc cheia[N];
      int e, ts, r1, n1, r2, n2;
      n = 1 + rand() % N;
      prepara(n, 0, 0);
      for (i = 0; i < n; i++) {
        f[i].original = rand() % 3 == 0;
        f[i].temPrim = rand() % 4 != 0;
        f[i].temSec = rand() % 2;
        verdade[i][0] = (unsigned char)(rand() % 5 == 0 ? FSINC_CONFIRMADA : rand() % 3 == 0 ? FSINC_NAO : 0);
        verdade[i][1] = (unsigned char)(rand() % 5 == 0 ? FSINC_CONFIRMADA : rand() % 3 == 0 ? FSINC_NAO : 0);
      }
      for (i = n - 1; i > 0; i--) { int j = rand() % (i + 1), t = fila[i]; fila[i] = fila[j]; fila[j] = t; }
      e = rand() % 2; ts = rand() % 2;
      memcpy(cheia, f, sizeof cheia);
      for (i = 0; i < n; i++) {   // a tabela toda de uma vez
        emb[i] = rand() % 6 == 0;
        if (cheia[i].temPrim) cheia[i].sincPrim = verdade[i][0];
        if (ts && cheia[i].temSec) cheia[i].sincSec = verdade[i][1];
        if (emb[i]) { cheia[i].temPrim = 1; cheia[i].sincPrim = FSINC_CONFIRMADA; }   // sincronizada de fabrica
      }
      g.embutida = embFalso;
      r2 = fonteauto_sincronizada(fila, n, cheia, e, ts, &n2);
      r1 = fonteauto_sincronizar(fila, n, f, e, ts, &g, &n1, &feitas);
      CONFERE(r1 == r2 && n1 == n2, "caso %d: em serie %d/%d, tabela cheia %d/%d", caso, r1, n1, r2, n2);
      CONFERE(feitas <= n, "conferiu mais fontes (%d) do que ha (%d)", feitas, n);
    }
  }

  if (falhas) { printf("fonteauto_sinc: %d falha(s)\n", falhas); return 1; }
  printf("fonteauto_sinc: ok\n");
  return 0;
}
