// A FILA DA FONTE AUTOMATICA E A VERIFICACAO UMA POR VEZ — issue #130.
//
// O relato: "depois de tocar Special Ops: Lioness uma vez, o painel do TorBox
// mostra 6 arquivos diferentes da Lioness carregados". Nao era o player: era a
// verificacao de streams.c, que conferia as candidatas EM PARALELO (4 fios,
// ate 8 candidatas). Conferir uma fonte de debrid nao e so perguntar: o GET com
// Range no link do AIOStreams faz o AIOStreams adicionar o torrent no TorBox,
// e o nosso proprio resolvedor (debrid.c) faz createtorrent. Cada candidata
// conferida vira um arquivo no painel e uma chamada na cota do mes — mesmo as
// que nunca iam tocar, porque a primeira ja servia.
//
// Aqui mora so a REGRA, sem rede, sem SDL e sem a lista de streams: quem
// monta a fila e em que ordem, e a garantia de que a verificacao para na
// primeira que serve. streams.c liga isto a rede; tests/fonteauto.c conta as
// URLs tocadas sem rede nenhuma.
#ifndef NV_FONTEAUTO_H
#define NV_FONTEAUTO_H

// Os dois modos de "Fonte automatica" em Ajustes. O INDICE e o gravado em
// ajustes.txt (fonteAutoLocal): nao reordenar.
//   MELHOR   = a regra de pontuacao do dono (MP4 4K DV > 4K > DV > altura),
//              conferindo em serie ate achar uma que serve.
//   PRIMEIRA = a primeira da lista, na ORDEM QUE O ADDON MANDOU, e so ela. E o
//              "Auto-play first source" do Nuvio: quem filtra e ordena no
//              AIOStreams ja decidiu, e conferir as outras so gasta cota.
enum { FONTEAUTO_MELHOR = 0, FONTEAUTO_PRIMEIRA = 1 };

// Monta em `fila` (capacidade `max`) as candidatas na ordem em que devem ser
// tentadas e devolve quantas entraram.
//   total      tamanho da lista de streams
//   preferida  indice lembrado pela fontepref (-1 = nenhum); entra PRIMEIRO
//   pontos     pontuacao de cada fonte (modo MELHOR); NULL = todas iguais
//   acimaTeto  1 = fonte acima de "Qualidade maxima" (modo PRIMEIRA: vai para
//              o fim, sem sair da fila); NULL = nenhuma
//   excluida   1 = ja falhou nesta lista, nao entra; NULL = nenhuma
// Em empate de pontuacao fica o de MENOR indice, que e a ordem do addon.
int fonteauto_fila(int modo, int total, int preferida, const long *pontos,
                   const unsigned char *acimaTeto, const unsigned char *excluida,
                   int max, int *fila);
// A MESMA FILA EM GRUPOS (#202, fonteregra.h): `grupo` de cada fonte,
// 0..FONTEAUTO_GRUPOS-1 na ordem em que os grupos entram (0 = add-on permitido
// que casa a regex; depois os que falham 1, 2... condicoes; depois "os outros"
// do mesmo jeito), -1 = nunca entra. Dentro de cada grupo vale a regra
// do modo, igual a de sempre. NULL = todas no grupo 0 (fonteauto_fila). A
// preferida continua na frente de tudo: e a escolha da pessoa neste titulo.
#define FONTEAUTO_GRUPOS 16   // 2 x (FR_FALTAS_MAX + 1)
int fonteauto_fila_g(int modo, int total, int preferida, const long *pontos,
                     const unsigned char *acimaTeto, const unsigned char *excluida,
                     const signed char *grupo, int max, int *fila);

// A MESMA FILA COM A ORDEM DOS ADD-ONS (fonteregra.h). `rank` = posicao do
// add-on de cada fonte na ordem (FR_ORDEM_SEM = fora dela); NULL = sem ordem.
// ordemUso: FR_ORDEM_NAO = a fila de sempre; FR_ORDEM_DESEMPATE = na mesma
// faixa de qualidade (pontos / FONTEAUTO_FAIXA) e no mesmo grupo vence o add-on
// mais cedo na ordem; FR_ORDEM_ESTRITA = dentro do grupo, as fontes dentro do
// teto do add-on mais cedo na ordem vem todas antes das do seguinte (e as
// acima do teto depois de todas as de dentro).
#define FONTEAUTO_FAIXA 10000
int fonteauto_fila_o(int modo, int total, int preferida, const long *pontos,
                     const unsigned char *acimaTeto, const unsigned char *excluida,
                     const signed char *grupo, const int *rank, int ordemUso,
                     int max, int *fila);

// AUDIO ORIGINAL ANTES DE QUALIDADE (dono, 2026-10-27): primeira volta = a fila inteira, na ordem dela,
// so as fontes que DIZEM ter o audio no idioma pedido (`temIdioma[indice]`); sem etiqueta ou "multi" ficam
// para a segunda volta, que e a fila de sempre do topo. Reordena `fila` sem perder nem repetir ninguem e
// devolve quantas passaram na primeira volta.
int fonteauto_idioma_primeiro(int *fila, int n, const unsigned char *temIdioma);

// ESCOLHA QUE MIRA UMA LEGENDA SINCRONIZADA (dono, 2026-10-27; HANDOFF §4 item 2). Achar uma legenda no idioma
// principal nao basta: tem de ser uma que SINCRONIZA com a fonte. A ordem do dono:
//   audio original > legenda principal sincronizada > legenda secundaria sincronizada
//   > legenda principal (sincronia nao confirmada) > legenda secundaria (idem).
// "Melhor qualidade" nao e um passo: e a ordem do addon, que `fila` ja traz (cada nivel percorre a fila de cima
// para baixo). Aqui mora so a REGRA, sem rede: quem le a sincronia (legref.c por Range) e quem troca de fonte
// e outro codigo. Passo sem idioma configurado e PULADO: sem secundaria nao ha os dois niveis dela; sem "Original"
// ligado (ou sem fonte que diga ter o original) o audio nao separa ninguem.
enum { FSINC_DESCONHECIDA = 0, FSINC_CONFIRMADA = 1, FSINC_NAO = 2 };   // NAO = conferida e nao casa, ou ilegivel
enum { FSINC_NADA = 0, FSINC_PRIM_OK = 1, FSINC_SEC_OK = 2, FSINC_PRIM = 3, FSINC_SEC = 4 };
typedef struct {
  unsigned char original;            // a fonte DIZ ter o audio original (a mesma leitura de temIdioma)
  unsigned char temPrim, temSec;     // ha legenda no idioma principal / secundario para ela
  unsigned char sincPrim, sincSec;   // FSINC_* dessa legenda com ESTA fonte (CONFIRMADA vale como "tem")
} FonteSinc;
// `fila` = indices de fonte na ordem de tentativa; `c` indexado pelo indice da fonte (como temIdioma). Devolve o
// indice escolhido e, em *nivel, FSINC_* (FSINC_NADA = ninguem tem legenda: a primeira da fila, como sempre).
// -1 se nao ha fila.
int fonteauto_sincronizada(const int *fila, int n, const FonteSinc *c, int exigeOriginal, int temSec, int *nivel);

// A CONFERENCIA EM SERIE (o laco do dono): percorre `fila` de cima para baixo, UMA fonte por vez, pergunta ao gancho
// se a legenda principal (slot 0) e a secundaria (slot 1, so com temSec) sincronizam com ela, e PARA na primeira em que
// a principal sincroniza (nada acima disso). Fonte que nao disputa (nao e original, com Original ligado e ao menos uma
// que o seja) nem e conferida: nao gasta leitura. Preenche sincPrim/sincSec de `c` (a conferida), devolve o melhor
// indice e o nivel como fonteauto_sincronizada. `conferidas` (se nao NULL) recebe quantas fontes foram conferidas.
// `conferir` e a rede (legref por Range + autosync_comparar): chamada primeiro com slot 0 e depois com slot 1 para a
// mesma fonte, em seguida, para quem guardar a referencia lida. `cancelado` e consultado antes de cada fonte.
// `embutida` (opcional): 1 se a fonte TRAZ uma faixa de dialogo no idioma principal dentro dela. Foi multiplexada com o
// video, entao e uma principal sincronizada de fabrica (degrau 1) sem precisar de legenda de addon; vale mesmo que o
// titulo nao tenha nenhuma. Perguntada antes de `conferir`, e a primeira que responder 1 encerra a procura.
typedef struct {
  int (*conferir)(int indice, int slot, void *u);   // FSINC_*
  int (*cancelado)(void *u);                         // NULL = nunca
  void *u;
  int (*embutida)(int indice, void *u);              // NULL = nao pergunta
} FonteSincGancho;
int fonteauto_sincronizar(const int *fila, int n, FonteSinc *c, int exigeOriginal, int temSec,
                          const FonteSincGancho *g, int *nivel, int *conferidas);

// Quantas candidatas o modo pode conferir numa escolha. PRIMEIRA = 1 sempre:
// a garantia do #130 e que so a fonte que vai tocar e tocada.
int fonteauto_tentativas(int modo, int pedidas);

// Confere a fila EM SERIE e PARA na primeira que serve: verificar() nunca e
// chamada para uma candidata depois de outra ter servido. *tocadas (se nao
// NULL) recebe quantas foram conferidas; falhou(), se nao NULL, e chamada para
// cada uma que nao serviu (streams.c a tira da fila da lista atual, e a
// proxima escolha nao a confere de novo). Devolve o indice ou -1.
typedef int  (*FonteVerificar)(int indice, void *u);
typedef void (*FonteFalhou)(int indice, void *u);
int fonteauto_primeira(const int *fila, int n, FonteVerificar verificar,
                       FonteFalhou falhou, void *u, int *tocadas);

// A ESCOLHA PODE SAIR ANTES DE TODOS OS ADDONS RESPONDEREM? (#221)
//
// Os vetores estao na ORDEM DE EXIBICAO da lista parcial (a ordem final da
// lista inteira, sem os addons que faltam). Devolve 1 quando:
//   - ninguem falta (e a escolha de sempre);
//   - a lembrada (preferida >= 0) ja esta na lista: ela vai na frente da fila
//     com qualquer lista, cheia ou parcial;
//   - PRIMEIRA: nenhum addon instalado ANTES do da primeira candidata falta
//     (a primeira da lista inteira nao pode mudar), e ela cabe no teto ou
//     ninguem mais falta;
//   - MELHOR: a melhor candidata presente e `boa` (dentro do teto, em cache,
//     na resolucao do teto — 4K com teto automatico);
//   - o prazo passou (prazoPassou).
// COM GRUPOS (#202): a candidata da frente nao sai enquanto falta um addon que
// ainda pode trazer um grupo MELHOR que o dela (pendenteGrupoMin < grupo dela)
// — e o "espera os add-ons permitidos; so sem fonte neles usa os outros". Com
// `instantaneo` ("Espera pelos add-ons" = Instantaneo) a primeira candidata
// do melhor grupo possivel sai na hora, sem pedir que ela seja "boa".
// E 0 quando nao ha candidata nao excluida, ou quando a lembrada pode estar
// num addon que ainda nao respondeu (prefPendente): ai nem o prazo vale — a
// escolha que a pessoa fez nao e trocada por pressa.
// `esperaTodos` ("Espera pelos add-ons" = Todos, PR #567): nada de "boa o
// bastante" nem de ordem; so decide quando nenhum add-on falta (ou a lembrada
// ja chegou), e ai a fila escolhe a melhor da lista inteira.
typedef struct {
  int modo, total, preferida, prefPendente, prazoPassou, algumPendente;
  const long *pontos;
  const unsigned char *acimaTeto, *excluida, *boa;
  const int *addon;                       // ordem do addon de cada fonte
  int (*pendenteAntes)(int addon, void *u);   // falta algum addon antes deste?
  void *u;
  const signed char *grupo;               // NULL = sem regras (todas no grupo 0)
  int pendenteGrupoMin;                   // melhor grupo que um pendente pode trazer
  int instantaneo;
  const int *rank;                        // NULL = sem ordem dos add-ons
  int ordemUso;                           // FR_ORDEM_*
  int pendenteRankMin;                    // menor posicao entre os add-ons pendentes (99 = nenhum)
  int esperaTodos;                        // "Todos os add-ons": so decide sem add-on pendente
} FonteautoParcial;
int fonteauto_pode_decidir(const FonteautoParcial *p);

#endif
