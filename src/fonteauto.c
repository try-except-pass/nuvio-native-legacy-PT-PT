#include "fonteauto.h"
#include <string.h>
#include <stdlib.h>
#include "fonteregra.h"
#include <stddef.h>

static int naFila(const int *fila, int n, int i) {
  int j;
  for (j = 0; j < n; j++) if (fila[j] == i) return 1;
  return 0;
}

static int foraDoGrupo(const signed char *grupo, int i, int g) {
  return grupo ? grupo[i] != g : g != 0;
}

// Rank do addon da fonte i (0 = primeiro da ordem); sem ordem, todas iguais.
static int rankDe(const int *rank, int i) { return rank ? rank[i] : 0; }
// Faixa de qualidade da pontuacao: floor(p / FONTEAUTO_FAIXA). Os degraus de
// resolucao e as multas de cache/teto/origem sao multiplos disso, entao duas
// fontes da mesma faixa sao "a mesma qualidade" para o desempate.
static long faixaDe(long p) { return p >= 0 ? p / FONTEAUTO_FAIXA : -((-p + FONTEAUTO_FAIXA - 1) / FONTEAUTO_FAIXA); }

int fonteauto_fila_o(int modo, int total, int preferida, const long *pontos,
                     const unsigned char *acimaTeto, const unsigned char *excluida,
                     const signed char *grupo, const int *rank, int ordemUso,
                     int max, int *fila) {
  int n = 0, g;
  if (!fila || max < 1 || total < 1) return 0;
  if (!rank) ordemUso = 0;

  // A PREFERIDA ENTRA PRIMEIRO nos dois modos: e a fonte que a pessoa escolheu
  // a mao neste titulo (issues #56 e #57). Continua sendo UMA candidata — no
  // modo PRIMEIRA ela e a unica conferida, e nenhuma outra e tocada.
  // Ela vale MAIS que a regex (o dono: "reuse last link" e a prioridade): a
  // regex so ordena as outras. Fora do escopo ela ja chega como -1 de
  // streams.c (posPref), nunca daqui.
  if (preferida >= 0 && preferida < total && !(excluida && excluida[preferida]))
    fila[n++] = preferida;

  for (g = 0; g < FONTEAUTO_GRUPOS && n < max; g++) {
    // Ordem estrita: dentro do teto antes de fora dele (o teto e preferencia,
    // nao filtro, ver cabeNoTeto em streams.c), e so depois a ordem dos
    // add-ons. Nos outros usos o MELHOR deixa o teto para a pontuacao.
    int passos = (modo == FONTEAUTO_PRIMEIRA || ordemUso == FR_ORDEM_ESTRITA) ? 2 : 1, passo;
    for (passo = 0; passo < passos && n < max; passo++) {
      // O proximo candidato deste (grupo, passo): o melhor pela chave do modo.
      while (n < max) {
        int melhor = -1, i;
        for (i = 0; i < total; i++) {
          int acima = acimaTeto && acimaTeto[i];
          if (passos == 2 && (passo == 0) == acima) continue;
          if ((excluida && excluida[i]) || foraDoGrupo(grupo, i, g) || naFila(fila, n, i)) continue;
          if (melhor < 0) { melhor = i; continue; }
          if (modo == FONTEAUTO_PRIMEIRA) {
            // Ordem do addon; com a ordem ligada, o addon mais cedo na ordem
            // vem antes (dentro dele, a ordem em que ele mandou).
            if (ordemUso && rankDe(rank, i) < rankDe(rank, melhor)) melhor = i;
          } else {
            long pi = pontos ? pontos[i] : 0, pm = pontos ? pontos[melhor] : 0;
            if (ordemUso == FR_ORDEM_ESTRITA) {
              int ri = rankDe(rank, i), rm = rankDe(rank, melhor);
              if (ri < rm || (ri == rm && pi > pm)) melhor = i;
            } else if (ordemUso == FR_ORDEM_DESEMPATE) {
              long fi = faixaDe(pi), fm = faixaDe(pm);
              int ri = rankDe(rank, i), rm = rankDe(rank, melhor);
              if (fi > fm || (fi == fm && (ri < rm || (ri == rm && pi > pm)))) melhor = i;
            } else if (pi > pm) melhor = i;   // `>` e nao `>=`: empate fica o primeiro da lista
          }
        }
        if (melhor < 0) break;
        fila[n++] = melhor;
      }
    }
  }
  return n;
}

int fonteauto_fila_g(int modo, int total, int preferida, const long *pontos,
                     const unsigned char *acimaTeto, const unsigned char *excluida,
                     const signed char *grupo, int max, int *fila) {
  return fonteauto_fila_o(modo, total, preferida, pontos, acimaTeto, excluida, grupo,
                          NULL, FR_ORDEM_NAO, max, fila);
}

int fonteauto_fila(int modo, int total, int preferida, const long *pontos,
                   const unsigned char *acimaTeto, const unsigned char *excluida,
                   int max, int *fila) {
  return fonteauto_fila_g(modo, total, preferida, pontos, acimaTeto, excluida, NULL, max, fila);
}

int fonteauto_tentativas(int modo, int pedidas) {
  if (modo == FONTEAUTO_PRIMEIRA) return 1;
  return pedidas < 1 ? 1 : pedidas;
}

int fonteauto_primeira(const int *fila, int n, FonteVerificar verificar,
                       FonteFalhou falhou, void *u, int *tocadas) {
  int q, conta = 0, escolhida = -1;
  if (fila && verificar)
    for (q = 0; q < n; q++) {
      conta++;
      if (verificar(fila[q], u)) { escolhida = fila[q]; break; }
      if (falhou) falhou(fila[q], u);
    }
  if (tocadas) *tocadas = conta;
  return escolhida;
}

int fonteauto_pode_decidir(const FonteautoParcial *p) {
  int fila[1], nf, c, q, temCand = 0;
  if (!p || p->total < 1) return 0;
  for (q = 0; q < p->total; q++)
    if (!(p->excluida && p->excluida[q]) && !(p->grupo && p->grupo[q] < 0)) { temCand = 1; break; }
  if (!temCand) return 0;
  if (!p->algumPendente) return 1;
  if (p->esperaTodos) return 0;      // "Todos os add-ons": falta um, espera
  if (p->preferida >= 0 && p->preferida < p->total &&
      !(p->excluida && p->excluida[p->preferida])) return 1;
  if (p->prefPendente) return 0;
  if (p->prazoPassou) return 1;
  nf = fonteauto_fila_o(p->modo, p->total, -1, p->pontos, p->acimaTeto,
                        p->excluida, p->grupo, p->rank, p->ordemUso, 1, fila);
  if (nf < 1) return 0;
  c = fila[0];
  // Um addon que ainda nao respondeu pode trazer um grupo melhor (o permitido):
  // espera por ele ate o prazo.
  if (p->grupo && p->pendenteGrupoMin < p->grupo[c]) return 0;
  if (p->instantaneo) return 1;
  // ORDEM ESTRITA: a escolha sai assim que o add-on mais cedo na ordem que ja
  // tem fonte valida respondeu, sem esperar os de depois. So falta esperar um
  // add-on que esta ANTES na ordem (ou, se a candidata nao esta na ordem, um
  // add-on da ordem). Fonte acima do teto espera o prazo, como em PRIMEIRA.
  if (p->ordemUso == FR_ORDEM_ESTRITA && p->rank) {
    int rc = p->rank[c];
    if (p->pendenteRankMin <= rc && p->pendenteRankMin < FR_ORDEM_SEM) return 0;
    if (rc < FR_ORDEM_SEM) return !(p->acimaTeto && p->acimaTeto[c]);
  }
  if (p->modo == FONTEAUTO_PRIMEIRA) {
    if (p->pendenteAntes && p->addon && p->pendenteAntes(p->addon[c], p->u)) return 0;
    return !(p->acimaTeto && p->acimaTeto[c]);
  }
  return p->boa && p->boa[c];
}

int fonteauto_nivel(const FonteSinc *s, int temSec) {
  if (s->sincPrim == FSINC_CONFIRMADA) return FSINC_PRIM_OK;
  if (temSec && s->sincSec == FSINC_CONFIRMADA) return FSINC_SEC_OK;
  if (s->sincPrim == FSINC_PROVAVEL) return FSINC_PRIM_PROV;
  if (temSec && s->sincSec == FSINC_PROVAVEL) return FSINC_SEC_PROV;
  if (s->temPrim) return FSINC_PRIM;
  if (temSec && s->temSec) return FSINC_SEC;
  return FSINC_NADA;
}

int fonteauto_classificar(const int *fila, int n, FonteSinc *c, int temSec, long *pontos, int *nivel) {
  static const long BASE[7] = { 0, 999999, 99999, 9999, 999, 99, 9 };
  long usados[7] = { 0 };
  int q, r, melhor = -1, primeira = -1;
  long melhorPts = -1;
  if (nivel) *nivel = FSINC_NADA;
  if (!fila || n < 1 || !c) return -1;
  // a gemea: sem referencia, e uma fonte CONFIRMADA com a mesma duracao (+-0,1 s) ja deu o veredito da mesma legenda
  for (q = 0; q < n; q++) {
    FonteSinc *s = &c[fila[q]];
    if (s->outro || !s->semRef) continue;
    for (r = 0; r < n; r++) {
      const FonteSinc *t = &c[fila[r]];
      long d = s->duracaoMs - t->duracaoMs;
      if (r == q || t->outro || s->duracaoMs <= 0 || t->duracaoMs <= 0 || d > 100 || d < -100) continue;
      if (s->temPrim && s->sincPrim != FSINC_CONFIRMADA && t->sincPrim == FSINC_CONFIRMADA) s->sincPrim = FSINC_PROVAVEL;
      if (temSec && s->temSec && s->sincSec != FSINC_CONFIRMADA && t->sincSec == FSINC_CONFIRMADA) s->sincSec = FSINC_PROVAVEL;
    }
    if (s->nomePrim && s->temPrim && s->sincPrim != FSINC_CONFIRMADA) s->sincPrim = FSINC_PROVAVEL;
    if (temSec && s->nomeSec && s->temSec && s->sincSec != FSINC_CONFIRMADA) s->sincSec = FSINC_PROVAVEL;
  }
  for (q = 0; q < n; q++) {
    const FonteSinc *s = &c[fila[q]];
    int nv;
    if (pontos) pontos[fila[q]] = 0;
    if (s->outro) continue;
    if (primeira < 0) primeira = fila[q];
    nv = fonteauto_nivel(s, temSec);
    if (!nv) continue;
    { long p = BASE[nv] - usados[nv]++;
      if (pontos) pontos[fila[q]] = p;
      if (p > melhorPts) { melhorPts = p; melhor = fila[q]; if (nivel) *nivel = nv; } }
  }
  return melhor >= 0 ? melhor : primeira;
}

int fonteauto_sincronizada(const int *fila, int n, const FonteSinc *c, int exigeOriginal, int temSec, int *nivel) {
  FonteSinc *copia;
  int r;
  (void)exigeOriginal;   // a regra do audio mora na conferencia (`outro`), nao mais na ordem
  if (nivel) *nivel = FSINC_NADA;
  if (!fila || n < 1 || !c) return -1;
  { int maior = 0;
    for (r = 0; r < n; r++) if (fila[r] + 1 > maior) maior = fila[r] + 1;
    copia = malloc(sizeof *copia * (size_t)maior);
    if (!copia) return -1;
    memcpy(copia, c, sizeof *copia * (size_t)maior); }
  r = fonteauto_classificar(fila, n, copia, temSec, NULL, nivel);
  free(copia);
  return r;
}

int fonteauto_sincronizar(const int *fila, int n, FonteSinc *c, int exigeOriginal, int temSec,
                          const FonteSincGancho *g, int *nivel, int *conferidas) {
  int q, feitas = 0, melhorNivel = 7;   // 7 = nada ainda
  if (conferidas) *conferidas = 0;
  if (nivel) *nivel = FSINC_NADA;
  if (!fila || n < 1 || !c || !g || !g->conferir) return -1;
  // UMA volta, de cima para baixo, na ordem da lista. Cada fonte: primeiro o AUDIO (etiqueta, ou as faixas do arquivo),
  // depois a legenda. Quem ja nao pode vencer o melhor achado (so um nivel melhor o supera: no mesmo nivel a de baixo
  // tem menos pontos) nao gasta leitura de legenda; a primeira principal sincronizada encerra a volta.
  for (q = 0; q < n; q++) {
    FonteSinc *s = &c[fila[q]];
    int v, nv;
    if (g->cancelado && g->cancelado(g->u)) break;
    if (exigeOriginal && !s->original && g->audioOriginal) {   // a etiqueta nao diz: le as faixas do arquivo
      int r = g->audioOriginal(fila[q], g->u);
      if (r == 1) s->original = 1;
      else if (r == 0) { s->outro = 1; continue; }
    }
    if (s->outro) continue;
    if (g->embutida && g->embutida(fila[q], g->u)) {   // sincronizada de fabrica: nada acima disso
      s->temPrim = 1; s->sincPrim = FSINC_CONFIRMADA;
      feitas++;
      break;
    }
    if (s->temPrim) {
      v = g->conferir(fila[q], 0, g->u);
      s->sincPrim = (unsigned char)v;
      if (v == FSINC_NAO && g->refAusente && g->refAusente(fila[q], &s->duracaoMs, g->u)) {
        s->semRef = 1;
        if (g->nomeCasa && g->nomeCasa(fila[q], 0, g->u)) s->nomePrim = 1;
      }
      if (v == FSINC_CONFIRMADA && g->duracao) s->duracaoMs = g->duracao(fila[q], g->u);
    }
    nv = fonteauto_nivel(s, temSec);
    if (nv && nv < melhorNivel) melhorNivel = nv;
    if (s->sincPrim == FSINC_CONFIRMADA) { feitas++; break; }
    if (temSec && s->temSec && melhorNivel > FSINC_SEC_OK) {   // so vale se ainda nao ha secundaria sincronizada acima
      v = g->conferir(fila[q], 1, g->u);
      s->sincSec = (unsigned char)v;
      if (v == FSINC_NAO && g->refAusente && g->refAusente(fila[q], &s->duracaoMs, g->u)) {
        s->semRef = 1;
        if (g->nomeCasa && g->nomeCasa(fila[q], 1, g->u)) s->nomeSec = 1;
      }
      if (v == FSINC_CONFIRMADA && g->duracao) s->duracaoMs = g->duracao(fila[q], g->u);
      nv = fonteauto_nivel(s, temSec);
      if (nv && nv < melhorNivel) melhorNivel = nv;
    }
    feitas++;
  }
  if (conferidas) *conferidas = feitas;
  return fonteauto_classificar(fila, n, c, temSec, NULL, nivel);
}

int fonteauto_idioma_primeiro(int *fila, int n, const unsigned char *temIdioma) {
  int com = 0, q;
  if (!fila || n < 2 || !temIdioma) return fila && n == 1 && temIdioma && temIdioma[fila[0]];
  int resto[n], nr = 0;
  for (q = 0; q < n; q++) {
    if (temIdioma[fila[q]]) fila[com++] = fila[q];
    else resto[nr++] = fila[q];
  }
  memcpy(fila + com, resto, sizeof *resto * (size_t)nr);
  return com;
}

int fonteauto_idioma_classes(int *fila, int n, const unsigned char *classe, int tirarOutros, int *declaradas) {
  int q, k = 0, c2 = 0;
  if (declaradas) *declaradas = 0;
  if (!fila || n < 1 || !classe) return n > 0 ? n : 0;
  { int copia[n];
    memcpy(copia, fila, sizeof *copia * (size_t)n);
    for (q = 0; q < n; q++) if (classe[copia[q]] == 2) fila[k++] = copia[q];
    c2 = k;
    for (q = 0; q < n; q++) if (classe[copia[q]] == 1) fila[k++] = copia[q];
    if (!tirarOutros) {
      for (q = 0; q < n; q++) if (classe[copia[q]] != 1 && classe[copia[q]] != 2) fila[k++] = copia[q];
      k = n;
    } }
  if (declaradas) *declaradas = c2;
  return k;
}
