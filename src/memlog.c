#include "memlog.h"
#include <stdio.h>
#include <string.h>
#ifdef __GLIBC__
#include <malloc.h>
#endif

static void (*avisoCb)(void);   // memlog_quando_aviso (fio principal)

#if defined(__linux__)
static int abaixo; // Compartilhado por FPS e player, ambos no fio principal.

static long lerKB(const char *path, const char *campo) {
  FILE *f = fopen(path, "r");
  char linha[256], chave[64], unidade[8], sobra;
  long kb = -1, valor;
  if (!f) return -1;
  while (fgets(linha, sizeof linha, f)) {
    if (sscanf(linha, "%63s %ld %7s %c", chave, &valor, unidade, &sobra) == 3 &&
        !strcmp(chave, campo) && !strcmp(unidade, "kB") && valor >= 0) {
      kb = valor;
      break;
    }
  }
  fclose(f);
  return kb;
}
// O piso ESCALA com a RAM, como tex_memoria_livre (150 MB e o da TV de 1965 MB). Numa de 1 GB, 150 MB livres e o repouso
// normal da home (150-180 MB); com o piso fixo o aviso disparava a cada volta do player e o corte de 30 s das texturas
// deixava o hero em branco (log da 2.0.23, 2026-10-27). Sem MemTotal legivel, vale o fixo.
static long pisoMb(void) {
  long kb = lerKB("/proc/meminfo", "MemTotal:");
  long ram = kb > 0 ? kb / 1024 : 0;
  return ram > 0 && ram < 1965 ? MEMLOG_PISO_MB * ram / 1965 : MEMLOG_PISO_MB;
}
#else
static long pisoMb(void) { return MEMLOG_PISO_MB; }
#endif

void memlog_amostra(char *sufixo, size_t tamanho) {
  if (!tamanho) return;
  sufixo[0] = 0;
#if defined(__linux__)
  long rss = lerKB("/proc/self/status", "VmRSS:");
  long disponivel = lerKB("/proc/meminfo", "MemAvailable:");
  if (rss < 0 || disponivel < 0) return;
  snprintf(sufixo, tamanho, " | mem=%.1f/%.1fMB", rss / 1024.0, disponivel / 1024.0);
  // o piso escalado nunca passa do fixo: so le o MemTotal quando ja esta abaixo do fixo (nada a mais por amostra)
  long piso = disponivel < MEMLOG_PISO_MB * 1024 ? pisoMb() : MEMLOG_PISO_MB;
  if (disponivel < piso * 1024 && !abaixo) {
    printf("[mem] aviso MemAvailable<%ldMB mem=%.1f/%.1fMB\n", piso, rss / 1024.0, disponivel / 1024.0);
    fflush(stdout);
    if (avisoCb) avisoCb();   // texturas + mkvass + texto + libass + heap
    else memlog_devolver();
  }
  abaixo = disponivel < piso * 1024;
#endif
}

int memlog_pouca_memoria(void) {
#if defined(__linux__)
  long kb = lerKB("/proc/meminfo", "MemAvailable:");
  return kb >= 0 && kb < pisoMb() * 1024;
#else
  return 0;
#endif
}

// Mesma regra do NvPlayer.poucaRamAparelho: ate 3 GiB de RAM total. So Android.
int memlog_android_fraco(void) {
  return 0;
}

long memlog_livre_mb(void) {
#if defined(__linux__)
  long kb = lerKB("/proc/meminfo", "MemAvailable:");
  return kb >= 0 ? kb / 1024 : -1;
#else
  return -1;
#endif
}

void memlog_evento(const char *evento) {
  char sufixo[96];
  memlog_amostra(sufixo, sizeof sufixo);
  if (*sufixo) {
    printf("[mem] %s%s\n", evento, sufixo + 2);
    fflush(stdout);
  }
}

// #415 (LG G4, webOS 9): a sessao de video deixava o rss em 524 MB com o player
// fechado e o gpu-cache parado — heap livre que o glibc segura. Devolve ao
// sistema no aviso e ao fechar o player. Sem glibc (Android, WASM), nada.
void memlog_devolver(void) {
#ifdef __GLIBC__
  malloc_trim(0);
#endif
}

void memlog_quando_aviso(void (*cb)(void)) { avisoCb = cb; }

int memlog_pode_reabrir(long (*livre)(void), void (*aliviar)(void)) {
  long mb = livre();
  if (mb < 0 || mb >= pisoMb()) return 1;
  aliviar();
  mb = livre();
  return mb < 0 || mb >= pisoMb();
}
