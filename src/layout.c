#include "layout.h"

int layout_modo_resolver(int preferencia, int automaticoMobile) {
  if (preferencia == 0) return 0;
  if (preferencia == 1) return 1;
  return automaticoMobile != 0;
}


void layout_tela_definir(int width, int height) { (void)width; (void)height; }
void layout_modo_definir(int mobile) { (void)mobile; }
int layout_modo_mobile(void) { return 0; }

