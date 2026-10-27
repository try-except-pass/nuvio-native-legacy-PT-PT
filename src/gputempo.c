// GPU time per frame via GL_EXT_disjoint_timer_query. See gputempo.h.
#include "gputempo.h"

void gputempo_iniciar(void) {}
void gputempo_quadro_inicio(void) {}
void gputempo_quadro_fim(void) {}
int gputempo_colher(double *med, double *pior, double *ult) {
  if (med) *med = 0.0;
  if (pior) *pior = 0.0;
  if (ult) *ult = 0.0;
  return 0;
}
double gputempo_p90(void) { return 0.0; }
double gputempo_ultimo(void) { return 0.0; }
