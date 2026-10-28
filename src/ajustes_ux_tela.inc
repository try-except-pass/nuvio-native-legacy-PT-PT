// Catalogo de apresentacao. Incluir depois de Item/SEC/ROT/OPC/MAIS.
// A ordem do enum, CHAVE e valor[] nao participa deste mapa: reordenar aqui nao
// mexe em nada que esta no disco ou na conta.
//
// 2.0.3 (estudo ajustes-ux, aprovado pelo dono em 07/10): as categorias seguem
// com os NOMES que a comunidade conhece, mas na ordem da tarefa — Assistir
// (Reproducao, Idiomas e legendas, Fontes e addons, TV ao vivo), Telas (Tela
// inicial, Aparencia, Pagina do titulo, Cartazes e trailers) e Conta e sistema
// (Conta e servicos, Esta TV, Sobre e ajuda). O que e tecnico fica DEPOIS do
// MAIS() da categoria: uma linha "Mais opcoes · N" fechada no fim da lista, que
// o OK abre no lugar (sem terceira tela). A pilula "Avancadas" do indice abre
// todas de uma vez. uxAvancada() le esta tabela; nao ha outra lista.
//
// 2.1 (Nuvio_GO, dono): 12 categorias — Destaque, Fileiras da Home, Galeria, TV ao vivo,
// Addons e fontes, Cartazes e conteudo, Pagina do titulo, Aparencia, Barra lateral,
// Idiomas e legendas, Reproducao, Conta e sobre. O que o dono fixou fica FORA da tela:
// FIXAS[] em ajustes.c (valor interno) e SO_FORA[] (botoes que perderam o sentido).
static const Item TELA[] = {
  SEC("Destaque", "O banner grande no alto da Home", "aj_layout-dashboard"),
    ROT("Destaque"),
      OPC(AJ_HERO), OPC(AJ_HERO_CHEIO), OPC(AJ_HERO_CATALOGOS),
      OPC(AJ_HERO_TAMANHO), OPC(AJ_HERO_DETALHES_PCT), OPC(AJ_HERO_SOMBRA_PCT),
      OPC(AJ_HERO_GENERO), OPC(AJ_HERO_SINOPSE), OPC(AJ_HERO_NOTAS), OPC(AJ_HERO_ANO), OPC(AJ_HERO_DURACAO),
      OPC(AJ_HERO_SINOPSE_TAM), OPC(AJ_HERO_TRANSICAO),
    ROT("Imagem"),
      OPC(AJ_PALCO_FUNDO), OPC(AJ_HERO_FUNDO), OPC(AJ_HERO_ARTE_DIF),
    ROT("Trailer do destaque"),
      OPC(AJ_HERO_TRAILER), OPC(AJ_HERO_TRAILER_SOM), OPC(AJ_LOGO_TRAILER), OPC(AJ_HERO_TRAILER_ESPERA),

  SEC("Fileiras da Home", "Layout, fileiras e Continuar assistindo", "aj_layout-dashboard"),
    ROT("Layout"),
      OPC(AJ_HOME_LAYOUT), OPC(AJ_FUNDO_DIN), OPC(AJ_FUNDO_DIN_COR), OPC(AJ_LANDSCAPE),
    ROT("Fileiras"),
      OPC(AJ_FIL_ORDEM), OPC(AJ_FIL_LIMITE), OPC(AJ_ITENS_FILEIRA),
      OPC(AJ_ESPACO_FILEIRAS), OPC(AJ_ESPACO_TITULOS), OPC(AJ_FIL_TITULOS),
      OPC(AJ_OCULTAR_NLANC), OPC(AJ_NOTAS_HOME), OPC(AJ_SELO_VISTO), OPC(AJ_ROTULOS),
      OPC(AJ_OCULTAR_FILMES_VISTOS), OPC(AJ_BIB_HOME),
      // A curadoria da fileira Heroi (#205): a fonte do cartao unico.
      OPC(AJ_CURADORIA_URL),
    ROT("Coleções"), OPC(AJ_COLECAO_EXIBICAO),
    ROT("Continuar assistindo"),
      OPC(AJ_CW_LIGADO), OPC(AJ_CW_ESTILO), OPC(AJ_CW_TAM_PCT), OPC(AJ_CW_THUMB), OPC(AJ_CW_BLUR_PROX),

  SEC("Galeria", "A segunda aba da Home, com os seus próprios ajustes", "aj_layout-dashboard"),
    ROT("Galeria"),
      OPC(AJ_MENU_GALERIA), OPC(AJ_GAL_ITENS_FILEIRA), OPC(AJ_GAL_ESPACO_FILEIRAS), OPC(AJ_GAL_ESPACO_TITULOS),
      OPC(AJ_GAL_FIL_TITULOS),

  SEC("TV ao vivo", "Provedor e solução de problemas ao assistir canais", "aj_tv-minimal-play"),
    ROT("Contas de TV ao vivo"),
      OPC(AJ_IPTV_ORDEM), OPC(AJ_IPTV_CONTA), OPC(AJ_IPTV_NOME), OPC(AJ_IPTV_ATIVA),
      OPC(AJ_IPTV_LISTA_STALKER), OPC(AJ_IPTV_ADD_STALKER),
      OPC(AJ_IPTV_LISTA_XTREAM), OPC(AJ_IPTV_ADD_XTREAM),
      OPC(AJ_IPTV_LISTA_M3U), OPC(AJ_IPTV_ADD_M3U),
      OPC(AJ_IPTV_REMOVER), OPC(AJ_IPTV_ACOES), OPC(AJ_IPTV_EXCLUIR),
      OPC(AJ_XTREAM_SERVIDOR), OPC(AJ_XTREAM_USUARIO), OPC(AJ_XTREAM_SENHA),
      OPC(AJ_XTREAM_CONTA), OPC(AJ_XTREAM_CONEXOES), OPC(AJ_XTREAM_LIMPAR), OPC(AJ_STALKER_PORTAL),
      OPC(AJ_STALKER_MAC), OPC(AJ_STALKER_LIMPAR),
      OPC(AJ_M3U_URL), OPC(AJ_M3U_EPG), OPC(AJ_M3U_LIMPAR),
      OPC(AJ_IPTV_SALVAR), OPC(AJ_IPTV_CANCELAR),
    ROT("Guia de TV"),
      OPC(AJ_GUIA_ORDEM), OPC(AJ_GUIA_FONTE), OPC(AJ_GUIA_RECARREGAR), OPC(AJ_EPG_PAIS), OPC(AJ_EPG_MEMORIA), OPC(AJ_GUIA_CATEGORIAS),
      OPC(AJ_URL_LISTA), OPC(AJ_URL_M3U_ADD), OPC(AJ_URL_XMLTV_ADD), OPC(AJ_URL_REM),
      OPC(AJ_LIVETV_SUGERIR),
    ROT("Se o canal não abre"),
      OPC(AJ_LIVETV_DIAG), OPC(AJ_LIVETV_RES), OPC(AJ_LIVETV_ESPERA),
    MAIS(),
    ROT("Se o canal não abre"),
      OPC(AJ_LIVETV_FORMATO), OPC(AJ_LIVETV_MODO), OPC(AJ_LIVETV_PROXY),

  SEC("Addons e fontes", "Addons, debrid e como o Nuvio escolhe a fonte", "aj_puzzle"),
    ROT("Addons"),
      OPC(AJ_ADDONS), OPC(AJ_PLUGINS), OPC(AJ_ADDONS_PRINCIPAL), OPC(AJ_BUSCA_CINEMETA), OPC(AJ_BUSCA_NUVIO),
    ROT("Escolha da fonte"),
      OPC(AJ_FILTROS_FONTES), OPC(AJ_FONTE_ORDEM), OPC(AJ_FONTE_ADDONS_PERM), OPC(AJ_FALHA_ADDON_ORDEM),
      OPC(AJ_FONTE_PRAZO), OPC(AJ_FONTE_REPOR),
      OPC(AJ_FONTE_GRUPO), OPC(AJ_FONTE_GRUPO_SESSOES), OPC(AJ_LINK_REUSAR), OPC(AJ_LINK_PRAZO),
      OPC(AJ_FONTE_AQUECER), OPC(AJ_FONTE_CONFERIR_VARIAS), OPC(AJ_FONTE_PREPARAR),
    ROT("Debrid"),
      OPC(AJ_DEBRID_RD), OPC(AJ_DEBRID_TB), OPC(AJ_DEBRID_PM), OPC(AJ_DEBRID_AD),
      OPC(AJ_DEBRID_AD_TESTAR), OPC(AJ_DEBRID_RD_TESTAR),
      OPC(AJ_DEBRID_TB_TESTAR), OPC(AJ_DEBRID_PM_TESTAR),

  SEC("Cartazes e conteúdo", "Cartaz em foco, de onde vem a arte e trailers", "aj_clapperboard"),
    ROT("Cartaz em foco"),
      OPC(AJ_EXPANDIR), OPC(AJ_EXPANDIR_ATRASO), OPC(AJ_BORDA_FOCO), OPC(AJ_SOMBRA_FOCO), OPC(AJ_BORDA_FOCO_BUSCA), OPC(AJ_MENU_CARTAZ),
      OPC(AJ_GRAD_CLASSICO), OPC(AJ_MURAL_TITULOS),
    ROT("Forma"),
      OPC(AJ_LARGURA_DP), OPC(AJ_RAIO_DP), OPC(AJ_PROF), OPC(AJ_PROF_BRILHO),
    ROT("De onde vem a arte"),
      OPC(AJ_ADDON_POSTER), OPC(AJ_ADDON_FUNDO), OPC(AJ_ADDON_LOGO), OPC(AJ_COL_ARTE_CONTA),
      OPC(AJ_POSTER_PROV), OPC(AJ_FANART_CHAVE), OPC(AJ_POSTER_INST), OPC(AJ_POSTER_TOKEN),
      OPC(AJ_POSTER_EXTRA), OPC(AJ_POSTER_CHAVE), OPC(AJ_POSTER_MODELO), OPC(AJ_POSTER_TESTAR),
    ROT("Trailers"),
      OPC(AJ_FOCO_TRAILER), OPC(AJ_DET_TRAILER_AUTO), OPC(AJ_DET_TRAILER),
      OPC(AJ_TRAILER_FONTE), OPC(AJ_TRAILER_QUAL), OPC(AJ_TRAILER_ASPECTO),

  SEC("Página do título", "O que aparece na página de cada título", "aj_file-text"),
    ROT("O que aparece"),
      OPC(AJ_DET_VEU), OPC(AJ_DET_BLUR_NAO_VISTOS), OPC(AJ_EP_TAMANHO), OPC(AJ_SINOPSE_COMPLETA),
    ROT("Dados do título"),
      OPC(AJ_TMDB_ARTE), OPC(AJ_DET_META_EXT),

  SEC("Aparência", "Vidro, texto, tamanhos e desempenho desta TV", "aj_palette"),
#ifdef NV_TECLADO_SIS
    OPC(AJ_TECLADO_ANDROID),
#endif
    ROT("Vidro"),
      OPC(AJ_VIDRO), OPC(AJ_VIDRO_CONTORNO), OPC(AJ_VIDRO_OPAC), OPC(AJ_VIDRO_FOSCO), OPC(AJ_VIDRO_QUALIDADE),
    ROT("Texto e tamanho"),
      OPC(AJ_FONTE_UI), OPC(AJ_TAMANHO_UI), OPC(AJ_TAMANHO_UI_PCT), OPC(AJ_ESCALA_TEXTO), OPC(AJ_ESCALA_TEXTO_PCT), OPC(AJ_DET_ESPACO), OPC(AJ_TAMANHO_AJUSTES), OPC(AJ_LAYOUT_AJUSTES),
      OPC(AJ_BUSCA_TECLADO_PCT), OPC(AJ_BUSCA_CARTAZ_PCT), OPC(AJ_BUSCA_ESPACO_PCT), OPC(AJ_BUSCA_CARTAZ_GAP_PCT),
    ROT("Relógio e pausa"),
      OPC(AJ_RELOGIO_PCT), OPC(AJ_PAUSA_ILHA_PCT),
    ROT("Imagem e desempenho"),
      OPC(AJ_RESOLUCAO), OPC(AJ_GPU_EFEITOS), OPC(AJ_AMPLIAR_UI), OPC(AJ_AMPLIACAO_ALGO), OPC(AJ_AMPLIACAO_NITIDEZ),
      OPC(AJ_QUALIDADE_IMG), OPC(AJ_NAV_RAPIDA), OPC(AJ_TEX_MB), OPC(AJ_ESPACO),
      OPC(AJ_DIAGNOSTICO), OPC(AJ_VELOCIDADE),

  SEC("Barra lateral", "Posição, tamanho e itens do menu", "aj_layout-dashboard"),
    ROT("Barra lateral"),
      OPC(AJ_MENU_TOPO), OPC(AJ_MENU_TAM_PCT), OPC(AJ_MENU_ICONE_PCT), OPC(AJ_MENU_ESPACO_PCT),
      OPC(AJ_MENU_GUIA), OPC(AJ_RAIL_BLUR),

  SEC("Idiomas e legendas", "Idioma da interface, legenda e metadados", "aj_languages"),
    ROT("Idiomas"),
      OPC(AJ_IDIOMA), OPC(AJ_LEG_LINGUA), OPC(AJ_LEG_LINGUA2), OPC(AJ_TMDB_IDIOMA),
      OPC(AJ_LEG_FORCADA), OPC(AJ_LEG_MOSTRAR), OPC(AJ_LEG_SYNC_AUTO),
    ROT("Legenda"),
      OPC(AJ_LEG1_TAMANHO), OPC(AJ_LEG1_FONTE), OPC(AJ_LEG1_COR), OPC(AJ_LEG1_OPACIDADE),
      OPC(AJ_LEG1_FUNDO), OPC(AJ_LEG1_POSICAO), OPC(AJ_LEG1_BORDA), OPC(AJ_LEG1_COR_BORDA),
      OPC(AJ_LEG1_NEGRITO), OPC(AJ_LEG1_ATRASO), OPC(AJ_LEG_CONTORNO_COR), OPC(AJ_LEG1_RESTAURAR),

  SEC("Reprodução", "O que o player faz durante e depois do vídeo", "aj_circle-play"),
    ROT("Durante e depois"),
      OPC(AJ_PULAR_ABERTURA), OPC(AJ_PULAR_RESUMO), OPC(AJ_PULAR_CREDITOS),
      OPC(AJ_PROXIMO_AUTO), OPC(AJ_PROXIMO_CONTAGEM),
      OPC(AJ_CW_CONCLUIDO), OPC(AJ_CW_MINIMO),
      OPC(AJ_SAIDA_PLAYER), OPC(AJ_PROPORCAO_PADRAO), OPC(AJ_BRILHO_PLAYER), OPC(AJ_HDR_UI),
    ROT("Miniatura ao avançar"),
      OPC(AJ_SEEKR_LIGADO), OPC(AJ_SEEKR_CHAVE), OPC(AJ_SEEKR_TESTAR), OPC(AJ_SEEKR_FITA),

  SEC("Conta e sobre", "Perfil, Trakt, Simkl, versão e registros", "aj_info"),
    ROT("Conta"),
      OPC(AJ_PERFIL_ATIVO), OPC(AJ_SYNC), OPC(AJ_PERFIL_EDITAR), OPC(AJ_SAIR),
      OPC(AJ_TRAKT), OPC(AJ_SIMKL), OPC(AJ_DISCORD),
    ROT("Sobre"),
      OPC(AJ_GUIA), OPC(AJ_NOVIDADES20), OPC(AJ_DICASCONTROLE), OPC(AJ_APOIAR),
      OPC(AJ_WEB_AJUSTES), OPC(AJ_AVISO_LEGAL),
    ROTS("App", ""), OPC(AJ_VERSAO_I), OPC(AJ_ATUALIZAR),
    ROTS("Registro", "para quem faz o app"),
      OPC(AJ_VER_REGISTRO), OPC(AJ_ENVIAR_LOG), OPC(AJ_LOG_REDE),
};
#define AJ_N_TELA (int)(sizeof TELA / sizeof *TELA)
#define AJ_MAX_SECOES 16

// Texto exibido no painel ao selecionar uma categoria. Mesma ordem dos SECs.
static const char *SECAO_AJUDA[] = {
  "Tamanho, detalhes, imagem e trailer do banner grande no alto da Home.",
  "Layout da Home, fileiras, coleções e Continuar assistindo.",
  "A segunda aba da Home (Galeria), com o seu próprio espaçamento e fileiras.",
  "Conexão dos provedores e diagnóstico de reprodução de canais ao vivo.",
  "Addons, debrid e como o Nuvio escolhe e confere a fonte.",
  "Cartaz em foco, de onde vem a arte e os trailers da Home e da página do título.",
  "O que aparece na página de cada título.",
  "Vidro, tipografia, tamanhos, relógio e desempenho desta TV.",
  "Posição, tamanho e itens da barra lateral.",
  "Idioma do app, da legenda e dos metadados, e o estilo da legenda.",
  "Saltar abertura e créditos, próximo episódio, proporção e miniaturas.",
  "Perfil, Trakt, Simkl, versão instalada, atualização e envio de registros.",
};

// AVANCADA = vem depois do MAIS() da propria categoria. Calculado uma vez em
// montarTela (ajustes.c); a busca e os atalhos perguntam por aqui.
static unsigned char uxAvancadaTab[AJ_N];
static void montarTela(void);
static int uxAvancada(int op) {
  montarTela();
  return op >= 0 && op < AJ_N && uxAvancadaTab[op];
}
