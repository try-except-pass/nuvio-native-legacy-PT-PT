// A CONFERENCIA REAL DE UMA FONTE CONTRA UMA LEGENDA (HANDOFF §4 item 2, fatia 2): "esta legenda sincroniza com ESTA
// fonte?". Le por HTTP Range a faixa de texto embutida da fonte (legref.c, fio e orcamento proprios, cancelavel) e
// compara com a legenda externa na engine do AutoSync (autosync_comparar, funcao pura): aceita = a legenda pode ser
// alinhada a esta fonte (offset, escala ou trechos), a mesma decisao que o legsync toma depois, com a fonte tocando.
//
// NAO toca o player, o overlay nem a fonte que esta tocando: so le e julga. Quem chama (fio proprio, fora do
// stream_primeira_boa_sessao, issue #130) decide o que fazer com o veredito.
#ifndef NV_FONTESINC_H
#define NV_FONTESINC_H
#include "legenda.h"
#include "legref.h"
#include <stdint.h>

typedef struct FonteSincLeitor FonteSincLeitor;
// `ler` NULL = HTTP Range (rede.c). Cria o fio da legref; nao faz rede.
FonteSincLeitor *fontesinc_leitor_criar(LegRefLer ler, void *u);
void fontesinc_leitor_destruir(FonteSincLeitor *l);

// Julga `doc` contra a faixa embutida de `url`. A referencia lida fica guardada: perguntar de novo pela MESMA url
// e sessao (outra legenda, o slot secundario) nao le a fonte outra vez. `sessao` tem de ser a mesma de `doc`
// (LegendaDocumentoInfo.sessao). `idiomaRef` = idioma preferido da faixa de referencia (ex. "en").
// Devolve FSINC_CONFIRMADA (aceita; *offsetMs = o aceito) ou FSINC_NAO (recusada, ou a referencia nao veio:
// sem faixa de texto, sem Range, rede, orcamento; o motivo em fontesinc_motivo). `cancelado` e consultado
// durante a leitura e a comparacao; cancelado devolve FSINC_DESCONHECIDA.
int fontesinc_julgar(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                     const LegendaDocumento *doc, int (*cancelado)(void *), void *u, int *offsetMs);
// A FAIXA EMBUTIDA da fonte, a mesma referencia que fontesinc_julgar usa (lida uma vez por url): uma faixa de dialogo
// no idioma da pessoa DENTRO da fonte foi multiplexada com o video e nao tem como estar dessincronizada.
// Devolve 1 = veio (*idioma = o da faixa lida, a mais proxima de `idiomaRef`; *dialogo = 1 se nao e forcada nem so
// letreiros), 0 = a fonte nao deu referencia, -1 = cancelado.
int fontesinc_faixa(FonteSincLeitor *l, const char *url, uint64_t sessao, const char *idiomaRef,
                    int (*cancelado)(void *), void *u, char *idioma, unsigned tam, int *dialogo);
// O motivo do ultimo veredito, em ingles estavel, para o log ("ok", "no_range", "low_confidence"...).
const char *fontesinc_motivo(const FonteSincLeitor *l);

#endif
