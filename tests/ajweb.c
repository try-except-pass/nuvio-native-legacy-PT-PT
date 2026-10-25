// Servidor de ajustes pelo navegador (src/ajweb.c) sem o app: os dois ganchos de
// ajustes.c sao simulados. O harness faz o papel do fio principal (chama
// ajweb_configurar em laco) enquanto tests/ajweb.sh fala com ele por curl.
#include "ajweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *ajustes_web_esquema(void) { return strdup("{\"perfil\":\"Teste\",\"s\":[]}"); }
int ajustes_web_aplicar(const char *chave, int v) { return !strcmp(chave, "a") && v == 3; }

int main(int argc, char **argv) {
  int i, seg = argc > 1 ? atoi(argv[1]) : 6;
  for (i = 0; i < seg * 100; i++) {
    ajweb_configurar(1);
    if (i == 20) { printf("URL %s\n", ajweb_url()); fflush(stdout); }
    usleep(10000);
  }
  ajweb_configurar(0);
  if (ajweb_url()[0]) { puts("FALHOU: url com o servidor desligado"); return 1; }
  return 0;
}
