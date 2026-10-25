// AJUSTES PELO NAVEGADOR (dono, 25/10/2026): "nao quero instalar uma versao nova
// na TV a cada teste". Ligado em Ajustes > Sobre e ajuda, a TV serve uma pagina
// na rede de casa — http://<ip-da-tv>:8098/<codigo>/ — com TODOS os ajustes da
// tela de Ajustes (a mesma tabela, os mesmos rotulos). Cada mudanca feita no
// navegador e gravada na hora, no PERFIL EM USO, e o efeito aparece na TV.
//
// Regras (as de logrede.h, mais um codigo porque aqui se ESCREVE):
//   - desligado por padrao; ligado, sobe um servidor HTTP minimo num fio
//     proprio. O fio NUNCA toca o estado do app: entrega o pedido ao fio
//     principal (ajweb_configurar, uma vez por quadro) e espera ate 4 s;
//   - so responde a enderecos da rede local (10/8, 172.16/12, 192.168/16) e ao
//     proprio aparelho, e so no caminho /<codigo>/...: 8 letras aleatorias, novas
//     a cada vez que o ajuste e ligado, e trocadas se alguem errar o caminho
//     AJWEB_ERROS_MAX vezes;
//   - o Host tem de ser o endereco mostrado e um Origin, se vier, o mesmo: sem
//     CORS e sem DNS rebinding;
//   - o que a pagina pode mudar e so o que a tela de Ajustes ja deixa mudar
//     (ajustes_web_aplicar); chaves, contas e listas ficam de fora;
//   - no .wgt (Emscripten) nao existe: o navegador nao abre porta.
#ifndef NV_AJWEB_H
#define NV_AJWEB_H

#define AJWEB_PORTA 8098        // ocupada: uma livre (ajweb_url diz qual)
#define AJWEB_ERROS_MAX 30

// Uma vez por quadro, no fio principal: liga/desliga o servidor conforme o
// ajuste e atende o pedido que o fio do servidor deixou na caixa.
void ajweb_configurar(int ligado);
// "http://192.168.1.20:8098/abcd2345/" com o servidor de pe; "" desligado.
const char *ajweb_url(void);

#endif
