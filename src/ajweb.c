// Ver ajweb.h para o porque e as regras.
#include "ajweb.h"

#ifdef __EMSCRIPTEN__
void ajweb_configurar(int ligado) { (void)ligado; }
const char *ajweb_url(void) { return ""; }
#else

#include "ajweb_pagina.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// Do ajustes.c (o .h dele puxa SDL; so estas duas interessam aqui).
char *ajustes_web_esquema(void);
int   ajustes_web_aplicar(const char *chave, int valor);

#define AW_CAB_MAX   8192
#define AW_CORPO_MAX 16384
#define AW_PRAZO_MS  5000     // um pedido inteiro, do accept ao fim do corpo
#define AW_ESPERA_S  4        // quanto o fio espera o fio principal

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static pthread_t fio;
static int vivo, ouvir = -1, acorda[2] = { -1, -1 }, porta, erros;
static char token[9], url[128], hostLan[64], hostLocal[64], origem[96];
static time_t ipQuando, proximaTentativa;
// A CAIXA: um pedido por vez, do fio do servidor para o principal.
static struct { int tipo, pronto; char *ent, *sai; } cx;   // tipo 1 = esquema, 2 = aplicar

const char *ajweb_url(void) { return vivo ? url : ""; }

// --- rede ------------------------------------------------------------------------
static int privado(in_addr_t a) {
  unsigned long h = ntohl(a);
  return (h >> 24) == 10 || (h >> 20) == 0xAC1 || (h >> 16) == 0xC0A8 || (h >> 24) == 127;
}

// A rota padrao diz qual interface a TV usa (connect num UDP so escolhe a rota,
// nao manda pacote); sem rota, a primeira interface IPv4 privada de pe.
static int ipLan(char *dst, size_t n) {
  struct sockaddr_in alvo, eu;
  socklen_t tam = sizeof eu;
  struct ifaddrs *lista = NULL, *i;
  int s = socket(AF_INET, SOCK_DGRAM, 0), ok = 0;
  in_addr_t achou = 0;
  if (s >= 0) {
    memset(&alvo, 0, sizeof alvo);
    alvo.sin_family = AF_INET;
    alvo.sin_port = htons(9);
    inet_pton(AF_INET, "192.0.2.1", &alvo.sin_addr);
    if (!connect(s, (struct sockaddr *)&alvo, sizeof alvo) &&
        !getsockname(s, (struct sockaddr *)&eu, &tam) && privado(eu.sin_addr.s_addr))
      achou = eu.sin_addr.s_addr;
    close(s);
  }
  if (!achou && !getifaddrs(&lista)) {
    for (i = lista; i; i = i->ifa_next) {
      if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET) continue;
      if (!(i->ifa_flags & IFF_UP) || (i->ifa_flags & IFF_LOOPBACK)) continue;
      if (privado(((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr)) {
        achou = ((struct sockaddr_in *)i->ifa_addr)->sin_addr.s_addr;
        break;
      }
    }
    freeifaddrs(lista);
  }
  if (achou) {
    struct in_addr a; a.s_addr = achou;
    ok = inet_ntop(AF_INET, &a, dst, (socklen_t)n) != NULL;
  }
  return ok;
}

// 31 simbolos sem os que se confundem; 8 = 39 bits. Rejeicao (248 = 8 x 31)
// para nao enviesar os primeiros.
static int gerarToken(void) {
  static const char A[] = "abcdefghjkmnpqrstuvwxyz23456789";
  unsigned char b[16];
  int f = open("/dev/urandom", O_RDONLY), i, k = 0;
  size_t lidos = 0;
  if (f < 0) return 0;
  while (lidos < sizeof b) {
    ssize_t r = read(f, b + lidos, sizeof b - lidos);
    if (r <= 0) { close(f); return 0; }
    lidos += (size_t)r;
  }
  close(f);
  for (i = 0; i < (int)sizeof b && k < 8; i++)
    if (b[i] < 248) token[k++] = A[b[i] % 31];
  token[k] = 0;
  memset(b, 0, sizeof b);
  return k == 8;
}

static void montarUrl(const char *ip) {
  snprintf(hostLan, sizeof hostLan, "%s:%d", ip, porta);
  snprintf(hostLocal, sizeof hostLocal, "127.0.0.1:%d", porta);
  snprintf(origem, sizeof origem, "http://%s", hostLan);
  snprintf(url, sizeof url, "http://%s/%s/", hostLan, token);
}

static void atualizarUrl(void) {
  char ip[48];
  ipQuando = time(NULL);
  if (ipLan(ip, sizeof ip)) montarUrl(ip);
}

// --- HTTP ------------------------------------------------------------------------
static long agoraMs(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000L + tv.tv_usec / 1000;
}

static int esperar(int fd, long prazo) {
  for (;;) {
    struct pollfd p[2];
    long falta = prazo - agoraMs();
    int r;
    if (falta <= 0) return 0;
    p[0].fd = fd; p[0].events = POLLIN; p[0].revents = 0;
    p[1].fd = acorda[0]; p[1].events = POLLIN; p[1].revents = 0;
    r = poll(p, 2, (int)(falta > 500 ? 500 : falta));
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 || (p[1].revents & POLLIN)) return 0;
    if (p[0].revents & (POLLIN | POLLHUP | POLLERR)) return 1;
  }
}

static void enviarTudo(int c, const char *b, size_t n) {
  while (n) {
    ssize_t w = send(c, b, n, MSG_NOSIGNAL);
    if (w <= 0) return;
    b += w; n -= (size_t)w;
  }
}

static void responder(int c, int cod, const char *tipo, const char *corpo, const char *extra) {
  char cab[900];
  const char *txt = cod == 200 ? "OK" : cod == 301 ? "Moved Permanently" : cod == 403 ? "Forbidden"
                  : cod == 404 ? "Not Found" : cod == 405 ? "Method Not Allowed"
                  : cod == 413 ? "Payload Too Large" : cod == 503 ? "Service Unavailable" : "Bad Request";
  size_t n = corpo ? strlen(corpo) : 0;
  int k = snprintf(cab, sizeof cab,
    "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
    "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n"
    "X-Frame-Options: DENY\r\n"
    "Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; "
    "connect-src 'self'; frame-ancestors 'none'\r\n%sConnection: close\r\n\r\n",
    cod, txt, tipo ? tipo : "text/plain; charset=utf-8", n, extra ? extra : "");
  enviarTudo(c, cab, (size_t)k);
  if (n) enviarTudo(c, corpo, n);
}

static int cabecalho(const char *cab, const char *nome, char *dst, size_t n) {
  size_t m = strlen(nome);
  const char *p = cab;
  while ((p = strstr(p, "\r\n")) != NULL) {
    p += 2;
    if (!strncasecmp(p, nome, m) && p[m] == ':') {
      const char *v = p + m + 1, *f;
      while (*v == ' ' || *v == '\t') v++;
      f = strstr(v, "\r\n");
      if (!f) f = v + strlen(v);
      snprintf(dst, n, "%.*s", (int)(f - v), v);
      return 1;
    }
  }
  return 0;
}

// Entrega o pedido ao fio principal e espera. NULL = o app nao respondeu a tempo
// (player aberto, tela travada): a pagina mostra "a TV esta ocupada".
static char *pedirAoApp(int tipo, char *entrada) {
  struct timespec lim;
  char *r = NULL;
  clock_gettime(CLOCK_REALTIME, &lim);
  lim.tv_sec += AW_ESPERA_S;
  pthread_mutex_lock(&trava);
  cx.tipo = tipo; cx.pronto = 0; cx.ent = entrada; cx.sai = NULL;
  while (!cx.pronto) {
    if (pthread_cond_timedwait(&cond, &trava, &lim)) break;
  }
  if (cx.pronto) r = cx.sai;
  cx.tipo = 0; cx.pronto = 0; cx.ent = NULL; cx.sai = NULL;
  pthread_mutex_unlock(&trava);
  return r;
}

// Fio principal: aplica "chave valor\n" linha a linha. Devolve {"ok":N,"falhou":[...]}.
static char *aplicarLinhas(const char *ent) {
  char *copia = strdup(ent ? ent : ""), *l, *sv = NULL, *r;
  size_t cap = 4096, n = 0;
  int ok = 0, nf = 0;
  if (!copia) return NULL;
  r = malloc(cap);
  if (!r) { free(copia); return NULL; }
  n = (size_t)snprintf(r, cap, "{\"falhou\":[");
  for (l = strtok_r(copia, "\n", &sv); l; l = strtok_r(NULL, "\n", &sv)) {
    char chave[96];
    int v;
    if (sscanf(l, "%95s %d", chave, &v) != 2) continue;
    if (ajustes_web_aplicar(chave, v)) { ok++; continue; }
    if (n + 120 < cap) {
      n += (size_t)snprintf(r + n, cap - n, "%s\"%.80s\"", nf ? "," : "", chave);
      nf++;
    }
  }
  snprintf(r + n, cap - n, "],\"ok\":%d}", ok);
  free(copia);
  return r;
}

void ajweb_configurar(int ligado);

static void atender(int c) {
  static char cab[AW_CAB_MAX + 1], corpo[AW_CORPO_MAX + 1];
  char metodo[8], caminho[160], host[96], orig[128], tam[24], pref[24];
  struct sockaddr_in par;
  socklen_t pl = sizeof par;
  long prazo = agoraMs() + AW_PRAZO_MS;
  size_t n = 0, fimCab, tem;
  char *sep, *q;
  memset(cab, 0, sizeof cab);
  if (getpeername(c, (struct sockaddr *)&par, &pl) || par.sin_family != AF_INET || !privado(par.sin_addr.s_addr)) {
    responder(c, 403, NULL, "Forbidden", NULL);
    return;
  }
  for (;;) {
    ssize_t k;
    if (n >= AW_CAB_MAX) { responder(c, 400, NULL, NULL, NULL); return; }
    if (!esperar(c, prazo)) return;
    k = recv(c, cab + n, AW_CAB_MAX - n, 0);
    if (k <= 0) return;
    n += (size_t)k; cab[n] = 0;
    if ((sep = strstr(cab, "\r\n\r\n")) != NULL) break;
  }
  fimCab = (size_t)(sep - cab) + 4;
  tem = n - fimCab;
  *sep = 0;
  if (sscanf(cab, "%7s %159s", metodo, caminho) != 2) { responder(c, 400, NULL, NULL, NULL); return; }
  if (!cabecalho(cab, "Host", host, sizeof host) || (strcmp(host, hostLan) && strcmp(host, hostLocal))) {
    puts("[ajweb] recusado: Host de fora");
    responder(c, 403, NULL, "Forbidden", NULL);
    return;
  }
  if (cabecalho(cab, "Origin", orig, sizeof orig)) {
    char o2[96];
    snprintf(o2, sizeof o2, "http://%s", hostLocal);
    if (strcmp(orig, origem) && strcmp(orig, o2)) {
      puts("[ajweb] recusado: Origin de fora");
      responder(c, 403, NULL, "Forbidden", NULL);
      return;
    }
  }
  if ((q = strchr(caminho, '?')) != NULL) *q = 0;
  snprintf(pref, sizeof pref, "/%s", token);
  n = strlen(pref);
  if (strncmp(caminho, pref, n) || (caminho[n] != 0 && caminho[n] != '/')) {
    if (strcmp(caminho, "/favicon.ico")) erros++;
    responder(c, 404, NULL, NULL, NULL);
    return;
  }
  if (caminho[n] == 0) {   // sem a barra final os caminhos relativos da pagina quebram
    char loc[160];
    snprintf(loc, sizeof loc, "Location: /%s/\r\n", token);
    responder(c, 301, NULL, NULL, loc);
    return;
  }
  if (!strcmp(caminho + n, "/")) {
    if (!strcmp(metodo, "GET")) puts("[ajweb] pagina aberta");
    if (strcmp(metodo, "GET") && strcmp(metodo, "HEAD")) { responder(c, 405, NULL, NULL, "Allow: GET, HEAD\r\n"); return; }
    responder(c, 200, "text/html; charset=utf-8", metodo[0] == 'G' ? AJWEB_PAGINA : NULL, NULL);
    return;
  }
  if (!strcmp(caminho + n, "/api/settings")) {
    char *r;
    if (!strcmp(metodo, "GET")) {
      r = pedirAoApp(1, NULL);
      if (!r) { responder(c, 503, "application/json", "{\"erro\":\"tv ocupada\"}", NULL); return; }
      responder(c, 200, "application/json; charset=utf-8", r, NULL);
      free(r);
      return;
    }
    if (!strcmp(metodo, "POST")) {
      long cl;
      if (!cabecalho(cab, "Content-Length", tam, sizeof tam)) { responder(c, 400, NULL, NULL, NULL); return; }
      cl = strtol(tam, NULL, 10);
      if (cl < 0 || cl > AW_CORPO_MAX) { responder(c, 413, NULL, NULL, NULL); return; }
      if (tem > (size_t)cl) tem = (size_t)cl;
      memcpy(corpo, cab + fimCab, tem);
      while ((long)tem < cl) {
        ssize_t k;
        if (!esperar(c, prazo)) return;
        k = recv(c, corpo + tem, (size_t)cl - tem, 0);
        if (k <= 0) return;
        tem += (size_t)k;
      }
      corpo[tem] = 0;
      r = pedirAoApp(2, corpo);
      if (!r) { responder(c, 503, "application/json", "{\"erro\":\"tv ocupada\"}", NULL); return; }
      responder(c, 200, "application/json; charset=utf-8", r, NULL);
      free(r);
      return;
    }
    responder(c, 405, NULL, NULL, "Allow: GET, POST\r\n");
    return;
  }
  erros++;
  responder(c, 404, NULL, NULL, NULL);
}

static void *servir(void *arg) {
  (void)arg;
  for (;;) {
    struct pollfd p[2];
    int r;
    p[0].fd = ouvir; p[0].events = POLLIN; p[0].revents = 0;
    p[1].fd = acorda[0]; p[1].events = POLLIN; p[1].revents = 0;
    r = poll(p, 2, 1000);
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 || (p[1].revents & POLLIN)) break;
    if (p[0].revents & POLLIN) {
      int c = accept(ouvir, NULL, NULL);
      if (c < 0) continue;
      atender(c);
      close(c);
    }
  }
  return NULL;
}

static void descer(void) {
  if (!vivo) return;
  if (acorda[1] >= 0) { char b = 1; ssize_t w = write(acorda[1], &b, 1); (void)w; }
  pthread_join(fio, NULL);
  vivo = 0;
  if (ouvir >= 0) { close(ouvir); ouvir = -1; }
  if (acorda[0] >= 0) { close(acorda[0]); close(acorda[1]); acorda[0] = acorda[1] = -1; }
  memset(token, 0, sizeof token);
  url[0] = 0;
  porta = 0; erros = 0;
  puts("[ajweb] desligado");
}

static int subir(void) {
  struct sockaddr_in a;
  socklen_t tam = sizeof a;
  char ip[48];
  int um = 1, tent;
  if (!ipLan(ip, sizeof ip)) { puts("[ajweb] sem IP de rede local"); return 0; }
  if (!gerarToken() || pipe(acorda)) { acorda[0] = acorda[1] = -1; return 0; }
  ouvir = socket(AF_INET, SOCK_STREAM, 0);
  if (ouvir < 0) goto falha;
  setsockopt(ouvir, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);
  for (tent = 0; tent < 2; tent++) {
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(tent == 0 ? AJWEB_PORTA : 0);
    if (!bind(ouvir, (struct sockaddr *)&a, sizeof a)) break;
  }
  if (tent == 2 || listen(ouvir, 4) || getsockname(ouvir, (struct sockaddr *)&a, &tam)) {
    printf("[ajweb] servidor nao subiu (errno %d)\n", errno);
    goto falha;
  }
  porta = ntohs(a.sin_port);
  montarUrl(ip);
  ipQuando = time(NULL);
  if (pthread_create(&fio, NULL, servir, NULL)) goto falha;
  vivo = 1;
  printf("[ajweb] no ar (porta %d)\n", porta);   // sem IP nem codigo no log
  return 1;
falha:
  if (ouvir >= 0) { close(ouvir); ouvir = -1; }
  if (acorda[0] >= 0) { close(acorda[0]); close(acorda[1]); acorda[0] = acorda[1] = -1; }
  memset(token, 0, sizeof token);
  return 0;
}

// Fio principal. A caixa so e lida aqui: e o unico lugar que toca os ajustes.
static void atenderCaixa(void) {
  pthread_mutex_lock(&trava);
  if (cx.tipo && !cx.pronto) {
    cx.sai = cx.tipo == 1 ? ajustes_web_esquema() : aplicarLinhas(cx.ent);
    cx.pronto = 1;
    pthread_cond_signal(&cond);
  }
  pthread_mutex_unlock(&trava);
}

void ajweb_configurar(int ligado) {
  time_t agora = time(NULL);
  if (!ligado) { descer(); return; }
  if (!vivo) {
    if (agora < proximaTentativa) return;
    if (!subir()) proximaTentativa = agora + 10;
    return;
  }
  if (erros >= AJWEB_ERROS_MAX) {   // alguem esta adivinhando o codigo: codigo novo
    puts("[ajweb] caminhos errados demais: codigo trocado");
    erros = 0;
    if (gerarToken()) atualizarUrl();
  }
  if (agora - ipQuando >= 30) atualizarUrl();   // a TV pode trocar de IP (DHCP)
  atenderCaixa();
}

#endif
