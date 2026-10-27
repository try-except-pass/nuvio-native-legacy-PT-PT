#ifndef NV_TELEFONEUI_H
#define NV_TELEFONEUI_H
#include "layout.h"
#include <math.h>

/* The explicit device mode controls touch UI; geometry is tested separately
 * by callers that need a portrait-specific layout. */
static inline int telefoneui_ativo(void) {
  return 0;
}
/* The caller supplies bounds in its own (possibly scaled) drawing layer. */
static inline float telefoneui_largura(float original, float tela, float margem) {
  return telefoneui_ativo() ? fminf(original, fmaxf(1, tela - 2 * margem)) : original;
}
#endif

