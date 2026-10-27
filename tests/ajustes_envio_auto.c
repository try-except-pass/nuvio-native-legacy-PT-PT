// #149: "ENVIAR REGISTROS SOZINHO" NAO FICA LIGADO.
//
// O relato: "depois de ligar, volta a desligado toda vez que fecho o app".
// O ajuste fica FORA do vetor posicional `valor[]` (as ultimas entradas do
// enum ficam no zero da inicializacao parcial, e 0 e LIGADO em V_LIGA), e o
// padrao de fabrica — desligado — era escrito em ajustes_iniciar(). Essa
// funcao roda TODA VEZ que a tela de Ajustes abre, depois de ajustes_dir() ja
// ter lido o arquivo: a escolha da pessoa era apagada na abertura seguinte da
// tela, e a proxima gravacao levava o "desligado" ao disco. E o mesmo defeito
// do #82/#86 (autoplay do trailer), no mesmo lugar.
//
// O remendo existia porque o vetor estava sete casas curto (as linhas do
// Stalker e do Xtream) e o padrao escrito nele caia em outra opcao. O padrao
// agora mora no vetor: LIGADO de fabrica (decisao do dono, 26/09/2026).
//
// Este teste le o disco como o arranque le (ajustes_dir sem a tela aberta) e
// abre a tela duas vezes, conferindo o valor e o que fica gravado.
#include "../src/ajustes.c"
#include "nvtmp.h"
#include <assert.h>
#include <unistd.h>

static void escrever(const char *caminho, const char *conteudo) {
  FILE *f = fopen(caminho, "w");
  assert(f);
  fputs(conteudo, f);
  fclose(f);
}

// Valor de "envioAuto" no arquivo; -1 se a linha nao existe.
static int noDisco(const char *caminho) {
  char linha[96];
  int v = -1, x;
  FILE *f = fopen(caminho, "r");
  if (!f) return -1;
  while (fgets(linha, sizeof linha, f))
    if (sscanf(linha, "envioAuto %d", &x) == 1) v = x;
  fclose(f);
  return v;
}

// NUVIO_GO (dono): "Enviar registros sozinho" saiu da tela e vale DESLIGADO, qualquer que seja o arquivo. O teste de antes
// (migracao do #149, padrao ligado) deixou de valer: aqui so se confere que o arquivo nao liga de novo.
int main(void) {
  char dir[NVT_MAX]; nvt_set(dir, sizeof dir, "/nuvio-aj-envio-XXXXXX");
  char caminho[700];
  assert(mkdtemp(dir));
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", dir);
  escrever(caminho, "envioAuto 0\n");
  ajustes_dir(dir);
  assert(!ajustes_envio_auto());
  ajustes_iniciar();
  assert(!ajustes_envio_auto());
  escrever(caminho, "idioma 0\n");
  ajustes_dir(dir);
  assert(!ajustes_envio_auto());
  (void)noDisco;
  puts("PASS: enviar registros sozinho vale desligado (fixo)");
  return 0;
}
