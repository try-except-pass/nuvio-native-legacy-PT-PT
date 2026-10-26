// Ver descanso.h.
#include "horafmt.h"
#include "descanso.h"
#include "esmaecer.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef DESCANSO_SEM_GFX
#include "gfx.h"
#include "layout.h"
#include "text.h"
#include "sinopse.h"
#include "tex_cache.h"
#include "catalogo.h"
#include "agenda.h"
#include "ajustes.h"
#include "idioma.h"
#include "app.h"
#include "home.h"
#endif

int descanso_item_serve(int temFundo, int temTitulo, int naLista, int progresso,
                        int fonte) {
  if (!temFundo || !temTitulo) return 0;
  if (fonte == DESC_FONTE_LISTA) return naLista || progresso > 0;
  return 1;
}

// Dois senos de periodo primo entre si: a rota so se repete depois de horas.
void descanso_rota(float s, float *px, float *py) {
  *px = 0.5f + 0.5f * sinf(s / 37.0f);
  *py = 0.5f + 0.5f * sinf(s / 53.0f + 1.3f);
}

#ifndef DESCANSO_SEM_GFX

#define DESC_ARTE_LARG   1920.0f
#define DESC_LOGO_W       820.0f
#define DESC_LOGO_H       220.0f
#define DESC_X            120.0f
#define DESC_BASE        (NV_TELA_H - 132.0f)
#define DESC_TEXTO_W     1080.0f
#define DESC_ESPERA_S       6.0f   // sem a arte seguinte pronta, quanto esperar no preto
#define DESC_ROTA_X        140.0f  // o bloco do relogio anda +-140 x +-80 px
#define DESC_ROTA_Y         80.0f
#define DESC_CICLO_MS      60000u  // o relogio fica num lugar por um minuto
#define DESC_TROCA_MS       1500u  // e leva 1,5 s para apagar e 1,5 s para acender

static int   ativoAnt;
static int   estiloAt = ESM_ESTILO_VITRINE;
static int   fonteAt;
static float tempoAtivo;           // segundos desde que a tela de descanso entrou
static int   fila[DESC_FILA_MAX];
static int   filaN, filaPos;
static unsigned filaRev;
static float itemT;                // segundos dentro do item atual
static float esperaT;              // preto esperando a arte seguinte
static int   atual = -1;           // indice de catalogo na tela (ou -1)
static GLuint texArte, texLogo, texProx;
static int   pedidoAbrir = -1;
static char  proxLinha[256];       // "Proxima estreia" do relogio, montada ao entrar

static unsigned sorteio(unsigned *e) { *e = *e * 1664525u + 1013904223u; return *e >> 8; }

static int itemServe(int i, int fonte) {
  const CatItem *c = cat_item(i);
  return c && descanso_item_serve(c->backdrop[0] != 0, c->titulo[0] != 0, c->naLista,
                                  c->progresso, fonte);
}

// Sorteia a ordem da sessao. "Minha lista e Continuar" com menos de tres
// titulos cai no catalogo inteiro: tres trocas ja repetem.
static void montarFila(unsigned semente) {
  int n = cat_n(), i, fonte = fonteAt;
  filaN = 0; filaPos = 0;
  if (fonte == DESC_FONTE_LISTA) {
    int conta = 0;
    for (i = 0; i < n; i++) if (itemServe(i, fonte)) conta++;
    if (conta < 3) fonte = DESC_FONTE_CATALOGO;
  }
  // dono (2026-10-26): a vitrine gira os mesmos titulos do destaque da Home ("Catalogos do destaque").
  { static int src[512];
    int k, m = home_hero_fonte_itens(src, (int)(sizeof src / sizeof *src));
    for (k = 0; k < m && filaN < DESC_FILA_MAX; k++)
      if (itemServe(src[k], DESC_FONTE_CATALOGO)) fila[filaN++] = src[k]; }
  if (filaN < 3) {   // automatico (ou pouca coisa): o catalogo, como antes
    filaN = 0;
    for (i = 0; i < n && filaN < DESC_FILA_MAX; i++)
      if (itemServe(i, fonte)) fila[filaN++] = i;
  }
  // Fisher-Yates sobre o que entrou.
  for (i = filaN - 1; i > 0; i--) {
    int j = (int)(sorteio(&semente) % (unsigned)(i + 1)), t = fila[i];
    fila[i] = fila[j]; fila[j] = t;
  }
  filaRev = cat_revisao();
}

static int itemDaFila(int pos) {
  int i;
  if (filaN <= 0) return -1;
  i = fila[((pos % filaN) + filaN) % filaN];
  return itemServe(i, DESC_FONTE_CATALOGO) ? i : -1;
}

static void montarProxima(void) {
  int i, n, melhor = 99, achou = -1;
  proxLinha[0] = 0;
  n = agenda_n();
  for (i = 0; i < n; i++) {
    const AgItem *a = agenda_lista(i);
    int d = a && a->dataProx[0] ? agenda_dias(a->dataProx) : AG_SEM_DATA;
    // Sem titulo a linha saia " · T2 E3 · amanha": estreia sem nome nao diz
    // nada a ninguem. A agenda preenche o titulo depois; ate la, fica a proxima.
    if (d >= 0 && d <= 7 && d < melhor && a->titulo[0]) { melhor = d; achou = i; }
  }
  if (achou >= 0) {
    const AgItem *a = agenda_lista(achou);
    char quando[64];
    agenda_falta(a->dataProx, quando, sizeof quando);
    if (a->temporada > 0 && a->episodio > 0)
      snprintf(proxLinha, sizeof proxLinha, i18n("%s · T%d E%d · %s"), a->titulo,
               a->temporada, a->episodio, quando);
    else snprintf(proxLinha, sizeof proxLinha, i18n("%s · %s"), a->titulo, quando);
  }
}

static void entrar(unsigned agora) {
  tempoAtivo = 0.0f;
  itemT = 0.0f; esperaT = 0.0f;
  atual = -1; texArte = texLogo = texProx = 0;
  montarFila(agora ^ 0x9e3779b9u);
  atual = itemDaFila(0);
  if (app_na_home() && agenda_n() == 0) agenda_montar();
  montarProxima();
}

int descanso_indice_em_tela(void) {
  if (estiloAt != ESM_ESTILO_VITRINE || atual < 0 || !texArte) return -1;
  if (itemT < 0.6f || itemT > DESC_ITEM_S - DESC_SAI_S) return -1;
  return atual;
}
void descanso_pedir_abrir(void) { pedidoAbrir = descanso_indice_em_tela(); }
int  descanso_pedido_abrir(void) { int v = pedidoAbrir; pedidoAbrir = -1; return v; }

void descanso_quadro(unsigned agora, float dt, int ativo, int estilo, int fonte) {
  const CatItem *c;
  if (dt < 0.0f) dt = 0.0f;
  if (dt > 0.1f) dt = 0.1f;
  estiloAt = estilo; fonteAt = fonte;
  if (!ativo) {
    if (ativoAnt) {
      printf("[descanso] sair t=%u dur=%.1fs estilo=%d item=%d tex=%u/%u/%u pedido=%d\n",
             agora, tempoAtivo, estilo, atual, texArte, texLogo, texProx, pedidoAbrir);
      fflush(stdout);
    }
    ativoAnt = 0; atual = -1; texArte = texLogo = texProx = 0; return;
  }
  if (!ativoAnt) {
    printf("[descanso] entrar t=%u estilo=%d fonte=%d catalogo=%d rev=%u\n",
           agora, estilo, fonte, cat_n(), cat_revisao());
    fflush(stdout);
    entrar(agora);
    printf("[descanso] pronto t=%u fila=%d item=%d\n", agora, filaN, atual);
    fflush(stdout);
  }
  ativoAnt = 1;
  tempoAtivo += dt;
  if (estilo != ESM_ESTILO_VITRINE) return;
  // Catalogo trocado no meio (sync, perfil): sorteia de novo, sem cortar o atual.
  if (filaRev != cat_revisao()) { montarFila(agora); filaPos = 0; }
  if (atual < 0) { atual = itemDaFila(filaPos); itemT = 0.0f; }
  c = atual >= 0 ? cat_item(atual) : NULL;
  texArte = c ? tex_obter_larg(c->backdrop, DESC_ARTE_LARG) : 0;
  texLogo = c && c->logo[0] ? tex_obter_logo_larg(c->logo, DESC_LOGO_W) : 0;
  // O relogio do item so anda com a arte na tela: sem isto o fade de entrada
  // gastaria o tempo baixando.
  if (texArte || !c) itemT += dt;
  // A arte do PROXIMO ja vai sendo pedida na metade do atual.
  { int prox = itemDaFila(filaPos + 1);
    const CatItem *p = prox >= 0 ? cat_item(prox) : NULL;
    texProx = p && itemT > DESC_ITEM_S * 0.5f ? tex_obter_larg(p->backdrop, DESC_ARTE_LARG) : 0;
    if (p && p->logo[0] && itemT > DESC_ITEM_S * 0.5f) (void)tex_obter_logo_larg(p->logo, DESC_LOGO_W);
    if (itemT >= DESC_ITEM_S) {
      // No preto ate a arte seguinte chegar; sem ela em DESC_ESPERA_S, pula.
      esperaT += dt;
      if (texProx || esperaT > DESC_ESPERA_S || !p) {
        filaPos += (texProx || !p) ? 1 : 2;
        atual = itemDaFila(filaPos);
        itemT = 0.0f; esperaT = 0.0f;
      }
    }
  }
}

static float suave(float t) {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  return t * t * (3.0f - 2.0f * t);
}

// Hora "HH:MM" e a data por extenso, recalculadas so quando o minuto vira.
static void horaAgora(char *hora, size_t nh, char *suf, size_t ns, char *data, size_t nd, float *segFrac) {
  static const char *DIA[7] = { "domingo", "segunda-feira", "ter\xc3\xa7" "a-feira", "quarta-feira",
                                "quinta-feira", "sexta-feira", "s\xc3\xa1" "bado" };
  time_t t = time(NULL);
  struct tm lt;
  hora[0] = 0; if (suf && ns) suf[0] = 0; if (data) data[0] = 0;
  if (!localtime_r(&t, &lt)) return;
  hora_tela_partes(hora, nh, suf, ns, &lt);
  if (data) snprintf(data, nd, i18n("%s, %d de %s"), i18n(DIA[lt.tm_wday]), lt.tm_mday,
                     i18n(agenda_mes_nome(lt.tm_mon + 1)));
  if (segFrac) *segFrac = (float)lt.tm_sec / 60.0f;
}

// --- RELOGIO -----------------------------------------------------------------
static void desenharRelogio(unsigned agora, float a) {
  char hora[12], suf[4], data[96];
  float seg, px, py, cx, cy, ar, ag, ab, s = (float)agora / 1000.0f;
  int reduz = ajustes_animacoes_reduzidas();
  TxtLinha lh, ld, lp;
  horaAgora(hora, sizeof hora, suf, sizeof suf, data, sizeof data, &seg);
  // O bloco troca de lugar A CADA CICLO (painel OLED) e fica PARADO dentro
  // dele, em pixel inteiro: andar um pouco a cada quadro deixava o numeral e a
  // data tremendo (texto em posicao fracionaria muda de amostragem todo quadro).
  // A troca nunca e um salto: nos ultimos DESC_TROCA_MS do ciclo o bloco
  // apaga devagar, muda de lugar no escuro e acende devagar no novo. Pedido do
  // dono, 05/10.
  { unsigned n = agora / DESC_CICLO_MS, f = agora % DESC_CICLO_MS;
    descanso_rota((float)n * 7.0f, &px, &py);
    a *= f < DESC_TROCA_MS ? suave((float)f / DESC_TROCA_MS)
       : f > DESC_CICLO_MS - DESC_TROCA_MS ? suave((float)(DESC_CICLO_MS - f) / DESC_TROCA_MS) : 1.0f; }
  cx = floorf(NV_TELA_W * 0.5f + (px * 2.0f - 1.0f) * DESC_ROTA_X);
  cy = floorf(NV_TELA_H * 0.5f + (py * 2.0f - 1.0f) * DESC_ROTA_Y);
  (void)s; (void)reduz;
  ajustes_acento(&ar, &ag, &ab);
  // Aurora: duas manchas muito fracas na cor de destaque, andando devagar.
  { float t = 0.0f;   // parada: so muda de lugar com o bloco
    GfxRect l1 = { cx - 900.0f + sinf(t * 0.11f) * 120.0f, cy - 640.0f, 1300.0f, 1300.0f };
    GfxRect l2 = { cx - 200.0f, cy - 760.0f + cosf(t * 0.09f) * 110.0f, 1200.0f, 1200.0f };
    gfx_rect(l1, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, ar, ag, ab, 0.10f * a);
    gfx_rect(l2, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f, ab, ar * 0.6f, ag, 0.06f * a); }
  // Brilho maximo do numeral em ~60% (OLED).
  lh = txt_linha(TXT_DESC_HORA, hora, 236, 238, 244, 255);
  if (lh.tex) {
    float x = floorf(cx - (float)lh.w * 0.5f), y = floorf(cy - (float)lh.h * 0.62f);
    txt_desenhar_alpha(lh, x, y, 0.62f * a);
    // 12 h: o "AM/PM" ao lado do numeral, na fonte comum (a do numeral so tem
    // digitos e ':'), alinhado pelo topo dos digitos. Nao entra na largura do
    // bloco: o numeral continua centrado como em 24 h.
    if (suf[0]) {
      TxtLinha ls = txt_linha(TXT_HEADLINE, suf, 236, 238, 244, 255);
      txt_desenhar_alpha(ls, x + (float)lh.w + 18.0f, y + (float)lh.h * 0.22f, 0.55f * a);
    }
    // A linha do minuto: trilho apagado e o trecho cheio no destaque.
    { float yl = y + (float)lh.h * 0.98f, w = (float)lh.w;
      gfx_cor((GfxRect){ x, yl, w, 2.0f }, 0, 1, 1, 1, 0.10f * a);
      gfx_cor((GfxRect){ x, yl, w * seg, 2.0f }, 0, ar, ag, ab, 0.70f * a);
      ld = txt_linha(TXT_HEADLINE, data, 200, 204, 212, 255);
      txt_desenhar_alpha(ld, floorf(cx - (float)ld.w * 0.5f), yl + 30.0f, 0.62f * a);
      if (proxLinha[0]) {
        TxtLinha k = txt_linha(TXT_CAPTION2, i18n("Próxima estreia"), 150, 154, 164, 255);
        lp = txt_linha_corta(TXT_CALLOUT, proxLinha, 210, 214, 222, 255, 1100.0f);
        txt_desenhar_alpha(k, floorf(cx - (float)k.w * 0.5f), yl + 30.0f + (float)ld.h + 40.0f, 0.5f * a);
        txt_desenhar_alpha(lp, floorf(cx - (float)lp.w * 0.5f),
                           yl + 30.0f + (float)ld.h + 40.0f + (float)k.h + 8.0f, 0.55f * a);
      } }
  }
}



static void desenharVitrine(unsigned agora, float a) {
  const CatItem *c = atual >= 0 ? cat_item(atual) : NULL;
  float t = itemT, ent, sai, vis, u;
  char hora[8];
  (void)agora;
  if (!c || !texArte) {
    if (filaN <= 0) desenharRelogio(agora, a);   // catalogo sem fundo nenhum
    return;
  }
  ent = suave(t / DESC_ENTRA_S);
  sai = t > DESC_ITEM_S - DESC_SAI_S ? suave((t - (DESC_ITEM_S - DESC_SAI_S)) / DESC_SAI_S) : 0.0f;
  vis = ent * (1.0f - sai) * a;
  u = 0.0f;
  // A arte PARADA, so com o cruzamento na troca. O Ken Burns (aproximacao
  // lenta e um passo para a esquerda) tremia na TV: a capa reamostrada em
  // posicao fracionaria a cada quadro. Pedido do dono, 05/10.
  { GfxRect r = { 0, 0, NV_TELA_W, NV_TELA_H };
    (void)u;
    gfx_tex_aspect_atual = tex_aspecto(c->backdrop);
    gfx_card_forcar_cover_atual = 1.0f;
    gfx_rect(r, texArte, GFX_CARD, 0, 0, 0, 0.0f, 1, 1, 1, vis);
    gfx_card_forcar_cover_atual = 0.0f;
    gfx_tex_aspect_atual = 0.0f; }
  // Veus: base forte (texto), topo leve (hora), e a sombra no canto do texto.
  // Cena clara nao pode apagar o titulo.
  gfx_rect((GfxRect){ -700.0f, 260.0f, 2300.0f, 1500.0f }, 0, GFX_SOMBRA, 1.0f, 0, 0, 0.5f,
           0, 0, 0, 0.80f * vis);
  gfx_rect((GfxRect){ 0, NV_TELA_H - 640.0f, NV_TELA_W, 640.0f }, 0, GFX_VEU_BAIXO,
           0, 0, 0, 0, 0, 0, 0, 0.94f * vis);
  gfx_rect((GfxRect){ 0, 0, NV_TELA_W, 240.0f }, 0, GFX_VEU_TOPO,
           0, 0, 0, 0, 0, 0, 0, 0.55f * vis);

  // dono (2026-10-26): so o titulo, no canto inferior esquerdo; nada de origem,
  // meta, sinopse nem dica de OK.
  { float y = DESC_BASE, k = suave((t - 1.0f) / 0.8f) * (1.0f - sai) * a;
    if (texLogo) {
      float ap = tex_aspecto(c->logo), w, h;
      if (ap <= 0.0f) ap = 4.0f;
      h = DESC_LOGO_H; w = h * ap;
      if (w > DESC_LOGO_W) { w = DESC_LOGO_W; h = w / ap; }
      y -= h;
      { GfxModo m = tex_marca_escura(c->logo) ? GFX_MARCA : GFX_TEXTO;
        gfx_rect((GfxRect){ DESC_X, y + (1.0f - k) * 14.0f, w, h }, texLogo, m,
                 0, 0, 0, 0.0f, 1, 1, 1, k); }
    } else {
      TxtLinha lt = txt_linha_corta(TXT_TITULO1, c->titulo, 255, 255, 255, 255, DESC_TEXTO_W);
      y -= (float)lt.h;
      txt_desenhar_alpha(lt, DESC_X, y + (1.0f - k) * 14.0f, k);
    }
  }

  // Sem hora na Vitrine (05/10, dono): numero parado no mesmo canto por
  // horas e o que marca painel OLED. So o quanto falta para o proximo.
  (void)hora;
  { float w = 150.0f, x = NV_TELA_W - 110.0f - w, y = NV_TELA_H - 92.0f;
    float p = t / DESC_ITEM_S;
    if (p > 1.0f) p = 1.0f;
    gfx_cor((GfxRect){ x, y, w, 3.0f }, 0.5f, 1, 1, 1, 0.14f * a);
    gfx_cor((GfxRect){ x, y, w * p, 3.0f }, 0.5f, 1, 1, 1, 0.70f * a); }
}

void descanso_desenhar(unsigned agora) {
  // O descanso aparece devagar sobre o preto, nunca de uma vez.
  float a = suave(tempoAtivo / 1.2f);
  if (a <= 0.0f) return;
  if (estiloAt == ESM_ESTILO_VITRINE) desenharVitrine(agora, a);
  else desenharRelogio(agora, a);
}

#endif
