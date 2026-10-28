// CATALOGO EM ESCALA: a escolha de fonte de verdade (streams.c + fontefalha.c) sobre milhares de titulos, com so a rede e o
// debrid simulados. Os casos vem de tools/catalogo-preparar.py (catalogo real colhido + sinteticos). Ver tests/catalogo_nucleo.sh.
#define _POSIX_C_SOURCE 200809L
#define SDL_MAIN_HANDLED
#include "../src/streams.c"
#include "stub_player_source_actions.h"
#include <assert.h>

typedef struct { int servico, arquivo; char hash[48]; } Pedido;
static Pedido pedidos[128];
static int nPedidos, nHttp, cancelar, trocar, errar, duplicar, recusar, bloquearManual;
static int falharServico = -1, falhaPct;
static int prioridadeTeste = 3, primeiraTeste;
static const char *qualidadeTeste = "Automatica";
static int nManual, manualResultado = 1, nP2P, p2pHabilitado, motor;
static AddEstado estadoAddons = ADD_PRONTO;
static unsigned disponiveis = 15;
static Stream substituta;
static char ultimoCab[512];

uint64_t perfis_sessao_geracao(void) { return 1; }
int perfis_sessao_atual(uint64_t geracao) { return geracao == 1; }
int perfis_sessao_travar(uint64_t geracao) { return geracao == 1; }
void perfis_sessao_destravar(void) {}
Uint32 SDL_GetTicks(void) { return 1000; }
int debrid_ativo(void) { return 1; }
int p2p_ativo(void) { return p2pHabilitado; }
const char *debrid_servico_nome(int s) {
  static const char *const nomes[] = {"Real-Debrid", "TorBox", "Premiumize", "AllDebrid"};
  return s >= 0 && s < 4 ? nomes[s] : "";
}
int debrid_servico_por_nome(const char *nome) {
  for (int s = 0; s < 4; s++) if (nome && !strcasecmp(nome, debrid_servico_nome(s))) return s;
  return -1;
}
int debrid_resolver_escolhido(const char *hash, int arq, char *url, unsigned tam,
                              char *servico, unsigned ns, int *pct) {
  nManual++;
  assert(!strcmp(hash, "MANUAL-HASH") && arq == 6);
  if (servico && ns) snprintf(servico, ns, "%s", debrid_servico_nome(DEBRID_TB));
  if (pct) *pct = 100;
  if (!manualResultado) { url[0] = 0; return 0; }
  snprintf(url, tam, "https://manual.native.invalid/%s/%d.mp4", hash, arq);
  return manualResultado;
}
int p2p_resolver(const char *hash, int arq, const char *fontes, char *url, unsigned tam) {
  (void)fontes; nP2P++;
  assert(!strcmp(hash, "MANUAL-HASH") && arq == 6);
  snprintf(url, tam, "%s", motor ? "http://127.0.0.1/embedded/file.mp4" : "https://p2p.invalid/torrent/file.mp4");
  return P2P_OK;
}
int p2p_ultimo_erro(void) { return 0; }
int p2pmotor_e_url(const char *url) { return url && strstr(url, "/embedded/") != NULL; }
int debrid_servico_disponivel(int s) { return !!(disponiveis & (1u << s)); }
AddEstado addons_estado(void) { return estadoAddons; }
int debrid_resolver_servico_cache(int s, const char *hash, int arq,
                                 char *url, unsigned tam, int *fora) {
  assert(nPedidos < (int)(sizeof pedidos / sizeof pedidos[0]));
  Pedido *p = &pedidos[nPedidos++];
  p->servico = s; p->arquivo = arq;
  snprintf(p->hash, sizeof p->hash, "%s", hash);
  if (trocar) stream_definir_lista(&substituta, 1);
  if (cancelar == 1) cancelar = 2;
  if (recusar == s + 1) disponiveis &= ~(1u << s);
  { unsigned h = 2166136261u; for (const char *q = hash; *q; q++) h = (h ^ (unsigned char)*q) * 16777619u;
    if ((int)(h % 100u) < falhaPct) { if (fora) *fora = 0; return 0; } }
  if (errar || s == falharServico || (recusar == s + 1)) { if (fora) *fora = 1; return 0; }
  if (duplicar && s == 0 && strcmp(hash, "hash-a") == 0)
    snprintf(url, tam, "https://native.invalid/shared.mp4");
  else snprintf(url, tam, "https://native.invalid/%d/%s/%d.mp4", s, hash, arq);
  return 1;
}
int debrid_resolver_cache(const char *hash, int arq, char *url, unsigned tam, int *fora) {
  return debrid_resolver_servico_cache(0, hash, arq, url, tam, fora);
}
int rede_url_final_tipo(const char *url, int seg, const char *const *cab,
                        char *fim, unsigned tam, int *status,
                        char *mime, unsigned mimeTam, long *corpo) {
  (void)seg; nHttp++;
  ultimoCab[0] = 0;
  if (cab && cab[0]) snprintf(ultimoCab, sizeof ultimoCab, "%s", cab[0]);
  snprintf(fim, tam, "%s", url);
  snprintf(mime, mimeTam, "video/mp4"); *status = 200; *corpo = 1024;
  return !errar;
}
char *rede_baixar_st(const char *u, int s, const char *const *c, int *st) {
  (void)u; (void)s; (void)c; (void)st; assert(!"no HLS in fixture"); return NULL;
}
int naovideo_mime(const char *m, const char *u, long b) { (void)m; (void)u; (void)b; return 0; }
int naovideo_nome(const char *a, const char *b, int h, long t) {
  (void)a; (void)b; (void)h; (void)t; return 0;
}
long fontesmortas_idade(const Stream *s) {
  return bloquearManual && strstr(s->url, "/manual.mp4") ? 0 : -1;
}
int ajustes_fonte_escopo(void) { return 0; }
int ajustes_fonte_regex_modo(void) { return 0; }
int ajustes_fonte_usar_outros(void) { return 0; }
int ajustes_fonte_prioridade(void) { return prioridadeTeste; }
int ajustes_fonte_hdr(void) { return 1; }
int ajustes_fonte_manual(void) { return 0; }
int ajustes_fonte_idioma_audio(void) { return 1; }
const char *extras_idioma_original_de(const char *imdb, int serie) { (void)imdb; (void)serie; return ling_original(); }
const char *extras_idioma_original_esperar(const char *i, int s, int p, int (*f)(void *), void *u) { (void)i; (void)s; (void)p; (void)f; (void)u; return ling_original(); }
int ajustes_fonte_preparar(void) { return 1; }
int ajustes_fonte_precarregar(void) { return 1; }
int ajustes_buffer_preset(void) { return 0; }
int ajustes_cache_seek_mb(void) { return 0; }
int ajustes_fonte_conferir_varias(void) { return 0; }
int ajustes_fonte_tocar_conferindo(void) { return 0; }
void debrid_contexto_capturar(DebridContexto *c) { memset(c, 0, sizeof *c); }
void debrid_contexto_entrar(const DebridContexto *c) { (void)c; }
void debrid_contexto_sair(void) {}
static int memoriaNextConsultas;
int fontepref_escolher_lista_next(const char *id, const Stream *s, int n,
                                  int (*pode)(int,int,void *), void *ctx) {
  (void)id;(void)s;(void)n;(void)pode;(void)ctx;memoriaNextConsultas++;return -1;
}
int ajustes_fonte_primeira(void) { return prioridadeTeste == 3 || primeiraTeste; }
int ajustes_fonte_ordem_uso(void) { return 2; }
int ajustes_dolby_vision(void) { return 0; }
int ajustes_dolby_atmos(void) { return 0; }
int ajustes_dv_mkv(void) { return 0; }
int ajustes_tamanho_max_gb(void) { return 0; }
int ajustes_tamanho_min_gb(void) { return 0; }
int ajustes_selos_coloridos(void) { return 0; }
const char *ajustes_qualidade(void) { return qualidadeTeste; }
unsigned fonteregra_versao(void) { return 1; }
int fonteregra_grupo(const FonteRegraCfg *c, const char *nome, int plugin, const char *texto) {
  (void)c; (void)plugin; (void)texto;
  return bloquearManual && strstr(nome, "Blocked") ? -1 : 0;
}
int fonteregra_no_escopo(const FonteRegraCfg *c, int p) { (void)c; (void)p; return 1; }
int fonteregra_ordem_rank(const char *p) { return p && !strncmp(p, "Addon ", 6) ? p[6] - 'A' : 0; }
uint64_t badges_bit(const char *s) { return !strcmp(s, "co-x265") ? UINT64_C(1) : 0; }
uint64_t badges_detectar(const char *s) {
  return s && (strstr(s, "HEVC") || strstr(s, "hevc")) ? UINT64_C(1) : 0;
}
int badges_fonte_hdr_marca(uint64_t b) { (void)b; return -1; }
uint64_t streamfit_agora_ms(void) { return 0; }
void streamfit_foto(StreamfitFoto *s, uint64_t ms) { (void)ms; memset(s, 0, sizeof *s); }
StreamfitClasse streamfit_classificar(const StreamfitFoto *f, const char *u,
                                      uint64_t b, double d, StreamfitResultado *s) {
  (void)f; (void)u; (void)b; (void)d; memset(s, 0, sizeof *s); return 0;
}
int selospacote_ativo(void) { return -1; }
unsigned selospacote_versao(void) { return 1; }
int selospacote_casar(const char *const *p, int n, unsigned short *o, int m) {
  (void)p; (void)n; (void)o; (void)m; return 0;
}
void selospacote_colorido(int c) { (void)c; }
int selospacote_embutidos_ok(void) { return 0; }
const SeloFiltro *selospacote_filtro(unsigned short id) { (void)id; return NULL; }

static int foiCancelado(void *ctx) { (void)ctx; return cancelar == 2; }

// ---------------------------------------------------------------------------------------------------------------------
#include <time.h>

typedef struct { int total, comOrig, semOrig, origDesconhecido, picks, fimSemFonte, avisos, falhasInvariante; } Estat;
static Estat est;
static FILE *surpresas;

static void falhou(const char *id, const char *quê, const char *detalhe) {
  est.falhasInvariante++;
  printf("INVARIANTE %s: %s %s\n", id, quê, detalhe ? detalhe : "");
}
static void surpresa(const char *id, const char *orig, const char *nome, int n, int marcados) {
  if (surpresas) fprintf(surpresas, "%s\t%s\t%s\tstreams=%d\tcom_original=%d\n", id, orig, nome, n, marcados);
}

typedef struct { int idx[256]; int n; int porAddon[4]; } Tentativas;
// A sessao devolve a PROXIMA fonte; aqui cada chamada de verificar e uma "abertura" e a fonte aberta e dada como falha (o
// teste so quer a ORDEM e os LIMITES): repete ate acabar. Devolve quantas fontes abriram.
static int percorrer(const FonteFalhaConfig *c, int n, Tentativas *t, int *fim) {
  FonteFalhaSessao *s = fontefalha_sessao_nova(c, n, -1);
  char url[4096];
  int idx, serv, r, abertas = 0;
  unsigned ger = stream_lista_geracao();
  memset(t, 0, sizeof *t);
  if (!s) return -1;
  for (;;) {
    r = stream_falha_verificar(s, ger, -1, foiCancelado, NULL, &idx, &serv, url, sizeof url);
    if (r != FONTEFALHA_PROXIMA) { *fim = r; break; }
    if (t->n < 256) t->idx[t->n++] = idx;
    abertas++;
    if (abertas >= 40) { *fim = FONTEFALHA_PROXIMA; break; }
  }
  fontefalha_sessao_liberar(s);
  return abertas;
}
static int indiceDoHash(const Stream *l, int n, const char *hash) {
  for (int i = 0; i < n; i++) if (!strcmp(l[i].infoHash, hash)) return i;
  return -1;
}

static void caso(const char *id, const char *orig, int naddons, int falha, const char *json) {
  Stream *l = NULL;
  int n = json ? stream_extrair(json, "Addon A", &l) : 0;
  FonteFalhaConfig c = {0};
  Tentativas t1, t2;
  int fim1 = -1, fim2 = -1, marcados = 0, origConhecida = strcmp(orig, "-") != 0;
  char det[160];
  est.total++;
  if (n < 0) n = 0;
  c.provedores = 0; c.addons = 1; c.nAddons = naddons;
  for (int p = 0; p < 4; p++) { c.ordem[p] = p; c.limites[p] = 3; }
  for (int a = 0; a < naddons; a++) {
    char u[80]; snprintf(u, sizeof u, "https://addon-%c.invalid/manifest.json", 'a' + a);
    fontefalha_addon_chave(u, c.addon[a].chave, sizeof c.addon[a].chave);
    c.addon[a].limite = FONTEFALHA_LINKS_ADDON;
  }
  for (int i = 0; i < n; i++) {   // reparte a lista entre os add-ons, mantendo a ordem de cada um
    int a = naddons > 1 ? (int)((unsigned)i * 2654435761u >> 28) % naddons : 0;
    snprintf(l[i].provedor, sizeof l[i].provedor, "Addon %c", 'A' + a);
    snprintf(l[i].addonChave, sizeof l[i].addonChave, "%s", c.addon[a].chave);
    l[i].addonOrdem = a;
    if (!l[i].infoHash[0]) snprintf(l[i].infoHash, sizeof l[i].infoHash, "%032x%08x", i, (unsigned)est.total);
  }
  falhaPct = falha;
  ling_local_audio("~"); ling_definir_original(origConhecida ? orig : "");
  stream_definir_lista(l, n);
  for (int i = 0; i < n; i++) marcados += temAudioPedido(&l[i]);
  nPedidos = 0;
  if (percorrer(&c, n, &t1, &fim1) < 0) { falhou(id, "sessao nao abriu", NULL); goto fim; }
  stream_sem_audio_original(det, sizeof det);   // zera o aviso para a segunda rodada: a primeira ja o consumiu abaixo
  {
    char idioma[24]; int avisou;
    // 2a rodada igual: a escolha tem de ser a mesma (determinismo)
    nPedidos = 0;
    stream_definir_lista(l, n);
    percorrer(&c, n, &t2, &fim2);
    avisou = stream_sem_audio_original(idioma, sizeof idioma);
    if (t1.n != t2.n || memcmp(t1.idx, t2.idx, sizeof(int) * (size_t)t1.n)) falhou(id, "nao deterministico", NULL);
    if (origConhecida && n > 0) {
      est.comOrig++;
      // 2026-10-28: a etiqueta de audio NAO reordena nem tira ninguem; a ordem e a do add-on (da lista, e entre add-ons,
      // a ordem deles). Quem tem o audio original e decidido depois, em segundo plano.
      if (marcados == 0) { est.semOrig++; surpresa(id, orig, id, n, 0); }   // so informativo: nenhuma DIZ ter
      if (avisou) falhou(id, "avisou sem original: a etiqueta nao exclui mais ninguem", NULL);
      for (int k = 1; k < t2.n; k++) {
        const Stream *a = &l[t2.idx[k - 1]], *b = &l[t2.idx[k]];
        if (a->addonOrdem > b->addonOrdem || (a->addonOrdem == b->addonOrdem && t2.idx[k - 1] > t2.idx[k])) {
          snprintf(det, sizeof det, "(%d antes de %d)", t2.idx[k - 1], t2.idx[k]); falhou(id, "fora da ordem do add-on", det); break;
        }
      }
      est.picks++;
    } else if (n > 0) {
      if (!origConhecida) est.origDesconhecido++;
      if (avisou) falhou(id, "avisou sem original com o idioma original desconhecido", NULL);
    }
    // limite de links por add-on: so conta o que foi aberto
    for (int k = 0; k < t2.n; k++) t2.porAddon[l[t2.idx[k]].addonOrdem & 3]++;
    for (int a = 0; a < 4; a++)
      if (t2.porAddon[a] > FONTEFALHA_LINKS_ADDON) { snprintf(det, sizeof det, "(add-on %d abriu %d)", a, t2.porAddon[a]); falhou(id, "passou do limite de links do add-on", det); }
    { int vistos[256] = {0}; for (int k = 0; k < t2.n; k++) { if (vistos[t2.idx[k] & 255]++) { falhou(id, "a mesma fonte duas vezes", NULL); break; } } }
    if (t2.n == 0 && n > 0 && !avisou) est.fimSemFonte++;
    if (avisou) est.avisos++;
  }
fim:
  stream_definir_lista(NULL, 0);
  free(l);
  (void)fim1; (void)fim2;
}

int main(int argc, char **argv) {
  FILE *f;
  char *linha = NULL, id[128] = "", orig[16] = "", tipo[16] = "";
  size_t cap = 0;
  int naddons = 1, falha = 0;
  clock_t t0 = clock();
  if (argc < 2) { fprintf(stderr, "uso: catalogo_nucleo <casos.txt> [surpresas.tsv]\n"); return 2; }
  f = fopen(argv[1], "r");
  if (!f) { perror(argv[1]); return 2; }
  if (argc > 2) surpresas = fopen(argv[2], "w");
  while (getline(&linha, &cap, f) > 0) {
    size_t L = strlen(linha);
    while (L && (linha[L - 1] == '\n' || linha[L - 1] == '\r')) linha[--L] = 0;
    if (linha[0] == 'T' && linha[1] == '\t') {
      if (sscanf(linha, "T\t%127[^\t]\t%15[^\t]\t%d\t%d\t%15s", id, orig, &naddons, &falha, tipo) != 5) { fprintf(stderr, "linha T ruim: %s\n", linha); return 2; }
    } else if (linha[0] == 'J' && linha[1] == '\t') {
      if (naddons < 1) naddons = 1;
      if (naddons > 3) naddons = 3;
      caso(id, orig, naddons, falha, linha + 2);
    }
  }
  free(linha); fclose(f);
  if (surpresas) fclose(surpresas);
  printf("catalogo: %d casos | original conhecido=%d (com fonte=%d, sem fonte com o original=%d) | idioma original desconhecido=%d | "
         "avisos=%d | invariantes violadas=%d | %.1fs\n",
         est.total, est.comOrig, est.picks, est.semOrig, est.origDesconhecido, est.avisos, est.falhasInvariante,
         (double)(clock() - t0) / CLOCKS_PER_SEC);
  return est.falhasInvariante ? 1 : 0;
}
