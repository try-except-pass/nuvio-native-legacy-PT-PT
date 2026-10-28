// A FONTE QUE SINCRONIZA COM A LEGENDA, EM SEGUNDO PLANO (dono, 2026-10-27; HANDOFF §4 item 2, fatia 2).
//
// A primeira fonte da escolha automatica abre JA (como sempre). Aqui, num fio proprio e sem esperar nada do player:
// espera a lista de legendas do titulo, baixa as do idioma principal (e do secundario), e percorre a fila da escolha,
// UMA fonte por vez (fonteauto_sincronizar + fontesinc_julgar: faixa embutida por Range contra a legenda), parando na
// primeira em que a legenda principal sincroniza. Se achar uma fonte MELHOR que a que toca (um degrau estritamente
// melhor da lista do dono: audio original, sincronia confirmada, legenda sem confirmacao), pede UMA troca; a UI a aplica pelo caminho de sempre, mantendo a posicao.
//
// So VOD de escolha automatica, so links diretos prontos (stream_sinc_link), nunca a fonte lembrada do titulo,
// nunca uma segunda troca na mesma reproducao. Nao cria nada dentro de stream_primeira_boa_sessao (issue #130).
#ifndef NV_FONTESINCAPP_H
#define NV_FONTESINCAPP_H
#include <stdint.h>

// Comeca (ou recomeca, cancelando a anterior) a conferencia para a fonte `atual`, recem-aberta pela escolha
// automatica da sessao de perfil `sessao`. Nao bloqueia.
void fontesincapp_iniciar(uint64_t sessao, int atual);
// Para o que estiver correndo (fim da reproducao, troca de titulo, fonte escolhida a mao).
void fontesincapp_cancelar(void);
// Chamada a cada quadro pela UI. 1 UMA vez quando ha uma fonte melhor: *indice = a fonte, *atual = a que tocava,
// *sincronizada = 1 se a legenda dela esta com a sincronia CONFIRMADA (0 = trocou por um degrau sem confirmacao).
int fontesincapp_trocar(int *indice, int *atual, int *sincronizada);

// 1 UMA vez quando a conferencia terminou e NENHUMA fonte tem legenda no idioma pedido (nem a embutida): a UI avisa
// "Legenda não encontrada" e segue tocando a primeira.
int fontesincapp_sem_legenda(void);

#endif
