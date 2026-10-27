// AUTO-SYNC DO APP SOBRE A AMOSTRA DE LEGENDAS PT-PT (tests/legendas_amostra): para cada titulo com a
// legenda PT escolhida e a inglesa do mesmo release, roda autosync_alinhar (o mesmo codigo do player)
// com a inglesa como referencia e imprime o que ele decidiu. O texto nao vem no arquivo de tempos; um
// texto unico por linha basta (o autosync so usa o texto para achar sinais e linhas repetidas).
// Uso: legsync_amostra <dir com tt*.pt.txt e tt*.en.txt> > saida.tsv
#include "autosync.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LegendaDocumento *ler(const char *arq, const char *idioma) {
  FILE *f = fopen(arq, "r");
  LegendaCue *v = NULL;
  int n = 0, cap = 0;
  double a, b;
  LegendaDocumentoInfo info;
  LegendaDocumento *d;
  if (!f) return NULL;
  while (fscanf(f, "%lf %lf", &a, &b) == 2) {
    if (n == cap) { cap = cap ? cap * 2 : 1024; v = realloc(v, sizeof *v * (size_t)cap); }
    memset(&v[n], 0, sizeof v[n]);
    v[n].inicio = a; v[n].fim = b; v[n].posX = v[n].posY = -1; v[n].cor = -1; v[n].ordem = n;
    snprintf(v[n].texto, sizeof v[n].texto, "linha numero %d da legenda", n);
    n++;
  }
  fclose(f);
  memset(&info, 0, sizeof info);
  snprintf(info.idioma, sizeof info.idioma, "%s", idioma);
  snprintf(info.identidade, sizeof info.identidade, "%s", arq);
  info.flags = LEGENDA_DOC_COMPLETO;
  d = legenda_documento_de_cues(v, n, &info);
  free(v);
  return d;
}

int main(int argc, char **argv) {
  DIR *dir;
  struct dirent *e;
  if (argc < 2 || !(dir = opendir(argv[1]))) return 2;
  printf("imdb\testado\tmotivo\ttipo\toffset_ms\tescala\tconfianca\tcobertura\ttempo_ms\n");
  while ((e = readdir(dir)) != NULL) {
    char id[32], pt[512], en[512];
    LegendaDocumento *dp, *de;
    if (sscanf(e->d_name, "%31[^.].pt.txt", id) != 1 || !strstr(e->d_name, ".pt.txt")) continue;
    snprintf(pt, sizeof pt, "%s/%s.pt.txt", argv[1], id);
    snprintf(en, sizeof en, "%s/%s.en.txt", argv[1], id);
    dp = ler(pt, "pt"); de = ler(en, "en");
    if (dp && de) {
      AutoSyncConfig cfg = autosync_config(AUTOSYNC_THOROUGH);
      AutoSyncMapa *m = NULL;
      AutoSyncResultado r = autosync_alinhar(dp, de, &cfg, NULL, NULL, &m);
      printf("%s\t%s\t%s\t%d\t%d\t%.4f\t%.3f\t%.3f\t%d\n", id,
             r.estado == AUTOSYNC_ACCEPTED ? "aceita" : r.estado == AUTOSYNC_REJECTED ? "recusada" : "outro",
             autosync_motivo(r.motivo), (int)r.tipo, r.offsetMs, r.escala, r.confianca, r.cobertura, r.tempoMs);
      autosync_mapa_liberar(m);
    }
    if (dp) legenda_documento_liberar(dp);
    if (de) legenda_documento_liberar(de);
  }
  closedir(dir);
  return 0;
}
