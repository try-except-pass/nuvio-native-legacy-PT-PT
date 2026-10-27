
  set -e
  SR=$NUVIO_SYSROOT
  RUST_LIBS=""
  if [ "$NUVIO_RUST_MKVCORE" = 1 ]; then RUST_LIBS=$NUVIO_RUST_LIB; fi
  ASS=$NUVIO_ASS_ROOT
  ASS_CFLAGS=""
  ASS_LIBS=""
  if [ "${NUVIO_ASS_LIBASS:-1}" = "1" ]; then
    [ -f "$ASS/include/ass/ass.h" ] || { echo "libass ARM ausente na imagem; reconstrua tools/Dockerfile ou use NUVIO_ASS_LIBASS=0" >&2; exit 2; }
    ASS_CFLAGS="-DNV_ASS_LIBASS -I$ASS/include"
    ASS_LIBS="-L$ASS/lib -Wl,--start-group -lass -lharfbuzz -lfribidi -lfreetype -Wl,--end-group"
  fi
  DTS_CFLAGS=""
  DTS_LIBS=""
  if [ "${NUVIO_DTS_FFMPEG:-1}" = "1" ]; then
    DTS=${NUVIO_DTS_ROOT:-/opt/nuvio-dts}
    [ -f "$DTS/include/libavcodec/avcodec.h" ] || { echo "FFmpeg DTS ausente; reconstrua tools/Dockerfile ou use NUVIO_DTS_FFMPEG=0" >&2; exit 2; }
    DTS_CFLAGS="-DNV_DTS_FFMPEG -DNV_DTS_WEBOS_RECOVERY -I$DTS/include"
    DTS_LIBS="-L$DTS/lib -Wl,--start-group -lavformat -lavcodec -lswresample -lavutil -Wl,--end-group -lpthread -lm"
  fi
  DV7_LIBS=""
  if [ "${NUVIO_DV7:-1}" = 1 ] && [ "${NUVIO_DTS_FFMPEG:-1}" = 1 ]; then
    export NUVIO_DV7_TARGET=armv7-unknown-linux-gnueabi
    [ -f build/dv7/libnuvio_dv7.a ] || bash tools/build-dv7.sh
    DTS_CFLAGS="$DTS_CFLAGS -DNV_DV7_CONVERSION"
    DV7_LIBS="build/dv7/libnuvio_dv7.a -ldl -lpthread -lm -lrt -Wl,-Bstatic -latomic -Wl,-Bdynamic -static-libgcc"
  fi
  # -DNV_WEBOS: a identidade do alvo tem que vir daqui, porque o compilador
  # webos define __linux__ igual a qualquer Linux e a toolchain nao tem macro
  # propria (src/ajustes.c separa o locale da TV por ela, como NV_TPK e
  # NV_ANDROID fazem nos outros alvos).
  P2P_CFLAGS=""
  P2P_LIBS=""
  if [ "${NUVIO_P2P_MOTOR:-}" = "1" ]; then
    P2P_CFLAGS="-DNV_P2P_MOTOR -I/p2p/nuvio-engine/include"
    # C++, libatomic e libgcc estaticos: o firmware da TV tem libstdc++ de outra
    # versao (ou nenhuma), e libatomic.so.1 nao e garantida.
    P2P_LIBS="/p2p/build-arm/libnuvio_engine.a /p2p/build-arm/_deps/nuvio_libtorrent-build/libtorrent-rasterbar.a $SR/usr/lib/libssl.a $SR/usr/lib/libcrypto.a -static-libgcc -Wl,-Bstatic -lstdc++ -latomic -Wl,-Bdynamic -lrt -Wl,--gc-sections"
  fi
  # #297: p2pmotor.c vai num objeto proprio com -D_FILE_OFFSET_BITS=64. O webOS
  # e ARM de 32 bits: sem a flag o lstat/readdir falham (EOVERFLOW) num arquivo
  # de torrent acima de 2 GB e o cache nunca era contado nem apagado. So este
  # arquivo: o header dele so expoe tipos de largura fixa (uint64_t), entao o
  # resto do binario segue sem a flag. O _Static_assert do .c barra o esquecimento.
  CC="arm-webos-linux-gnueabi-gcc"
  if command -v ccache >/dev/null 2>&1; then CC="ccache $CC"; fi
  $CC -c src/p2pmotor.c -o $TMPW/p2pmotor.o -O2 -DNV_WEBOS -D_FILE_OFFSET_BITS=64 $NUVIO_EXTRA_CFLAGS $P2P_CFLAGS \
    -I$SR/usr/include -I$SR/usr/include/SDL2
  printf "%s\n" -O2 -DNV_WEBOS $NUVIO_EXTRA_CFLAGS $ASS_CFLAGS $P2P_CFLAGS $DTS_CFLAGS \
    -I$SR/usr/include -I$SR/usr/include/SDL2 > $TMPW/arm-flags
  for k in NV_SUPABASE_URL NV_SUPABASE_ANON_KEY NV_TV_LOGIN_BASE NV_TRAKT_CLIENT_ID \
           NV_TRAKT_CLIENT_SECRET NV_SIMKL_CLIENT_ID NV_SIMKL_APP NV_TMDB_API_KEY NV_SEEKR_API_KEY \
           NV_REC_URL NV_DISCORD_CLIENT_ID NV_VERSAO; do
    v=${!k}
    v=$(printf "%s" "$v" | sed "s/[\\\\\"]/\\\\&/g")
    printf "%s\n" "\"-D$k=\\\"$v\\\"\""
  done >> $TMPW/arm-flags
  KEY=$( { cat $TMPW/arm-flags; sha256sum tools/arm.sh tools/native-objects.mk; printf "%s" "$NUVIO_SDK_ID"; } | sha256sum | cut -d" " -f1)
  OBJDIR="build/arm-objects/$KEY"
  SOURCES=$(ls src/*.c src/dts/*.c | grep -v "^src/p2pmotor\.c$")
  make -s -f tools/native-objects.mk -j "$NUVIO_BUILD_JOBS" CC="$CC" \
    OBJDIR="$OBJDIR" SOURCES="$SOURCES" NATIVE_FLAGS=@$TMPW/arm-flags
  if command -v ccache >/dev/null 2>&1; then ccache -s; fi
  OBJECTS=()
  for f in $(ls src/*.c | grep -v "^src/p2pmotor\.c$") src/dts/*.c; do
    OBJECTS+=("$OBJDIR/${f%.c}.o")
  done
  arm-webos-linux-gnueabi-gcc "${OBJECTS[@]}" $TMPW/p2pmotor.o -o nuvio-proto.arm -O2 -DNV_WEBOS $NUVIO_EXTRA_CFLAGS $ASS_CFLAGS $P2P_CFLAGS $DTS_CFLAGS \
    -DNV_SUPABASE_URL="\"$NV_SUPABASE_URL\"" \
    -DNV_SUPABASE_ANON_KEY="\"$NV_SUPABASE_ANON_KEY\"" \
    -DNV_TV_LOGIN_BASE="\"$NV_TV_LOGIN_BASE\"" \
    -DNV_TRAKT_CLIENT_ID="\"$NV_TRAKT_CLIENT_ID\"" \
    -DNV_TRAKT_CLIENT_SECRET="\"$NV_TRAKT_CLIENT_SECRET\"" \
    -DNV_SIMKL_CLIENT_ID="\"$NV_SIMKL_CLIENT_ID\"" \
    -DNV_SIMKL_APP="\"$NV_SIMKL_APP\"" \
    -DNV_TMDB_API_KEY="\"$NV_TMDB_API_KEY\"" \
    -DNV_SEEKR_API_KEY="\"$NV_SEEKR_API_KEY\"" \
    -DNV_REC_URL="\"$NV_REC_URL\"" \
    -DNV_DISCORD_CLIENT_ID="\"${NV_DISCORD_CLIENT_ID:-}\"" \
    -DNV_VERSAO="\"$NV_VERSAO\"" \
    -I$SR/usr/include -I$SR/usr/include/SDL2 \
    $RUST_LIBS -lSDL2 -lSDL2_image -lSDL2_ttf -lGLESv2 -lEGL $P2P_LIBS -ldl -lpthread -lz -lm $ASS_LIBS $DTS_LIBS $DV7_LIBS \
    -Wl,--export-dynamic-symbol=pthread_create -Wl,--exclude-libs,ALL
  # #317: --exclude-libs,ALL tira da tabela dinamica tudo o que vem de arquivo
  # .a (FreeType/harfbuzz do libass, libstdc++ do P2P, FFmpeg). Sem ele o ld
  # exportava 178 FT_* (+ TT_RunIns/TT_New_Context) do nosso FreeType 2.13,
  # porque a libfreetype.so do sistema define os mesmos nomes. No webOS 4.0 o
  # SDL_ttf da LG carrega a FreeType 2.6.5 do sistema por dlsym, e as chamadas
  # internas dela (FT_Add_Default_Modules, FT_New_Library...) caiam nas NOSSAS:
  # face com FT_Face_Internal de 2.6.5 (refcount=1 em +0x34) lida pelo nosso
  # tt_glyph_load como incremental_interface -> SIGSEGV addr=0x1 na partida.
  # O pthread_create acima vem de objeto nosso, nao de .a, e segue exportado.
  # src/dts/rt_shim.c: the LG audio sink (KADP_OSA_CreateThread) must bind to
  # our pthread_create, which retries its real-time thread with default
  # attributes instead of letting the firmware crash on EINVAL (LG C9 core).
  rm -rf deploy/app/licenses/dv7
  if [ -n "$DV7_LIBS" ]; then
    mkdir -p deploy/app/licenses/dv7
    cp build/dv7/licenses/* build/dv7/SOURCE.txt build/dv7/Cargo.lock deploy/app/licenses/dv7/
  fi
  mkdir -p deploy/app/lib
  find deploy/app/lib -maxdepth 1 -type f -name "dts-starfish-webos*.so" -delete
  if [ -d deploy/app/licenses/dts ]; then
    find deploy/app/licenses/dts -maxdepth 1 -type f \
      \( -name "COPYING.LGPLv2.1" -o -name "SOURCE.txt" \) -delete
  fi
  if [ "${NUVIO_DTS_FFMPEG:-1}" = "1" ]; then
    bash tools/build-dts-pipeline.sh --in-container --output deploy/app/lib
    mkdir -p deploy/app/licenses/dts
    cp "$DTS/COPYING.LGPLv2.1" "$DTS/SOURCE.txt" deploy/app/licenses/dts/
  fi