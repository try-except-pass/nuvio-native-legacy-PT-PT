#include "redemarca.h"
#include "streamfit.h"
#include <pthread.h>

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static uint64_t sequenciaAtual, marcaAtual;
uint64_t redemarca_atual(void) {
  uint64_t v;
  pthread_mutex_lock(&trava); v = marcaAtual; pthread_mutex_unlock(&trava);
  return v;
}
void redemarca_observar(uint64_t seq, int conhecida) {
  pthread_mutex_lock(&trava);
  if (seq && (seq > sequenciaAtual || (seq == sequenciaAtual && !conhecida))) {
    sequenciaAtual = seq; marcaAtual = conhecida ? seq : 0;
    /* Publish invalidation in the same critical section as the epoch: a
     * worker cannot read a new epoch while the engine still has the old one. */
    streamfit_rede(marcaAtual);
  }
  pthread_mutex_unlock(&trava);
}
