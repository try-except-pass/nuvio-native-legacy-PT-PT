# Build do .ipk da LG neste PC, sem docker: `source tools/lg-host/env.sh && bash tools/arm-host.sh --ipk --build`.
export SDK=$HOME/opt/arm-webos-linux-gnueabi_sdk-buildroot
export PATH=$SDK/bin:$HOME/opt/rust/cargo/bin:$PATH
export RUSTUP_HOME=$HOME/opt/rust/rustup CARGO_HOME=$HOME/opt/rust/cargo
export NUVIO_SYSROOT=$SDK/arm-webos-linux-gnueabi/sysroot NUVIO_ASS_ROOT=$HOME/opt/nuvio-ass NUVIO_DTS_ROOT=$HOME/opt/nuvio-dts
export NUVIO_DTS_CC=$SDK/bin/arm-webos-linux-gnueabi-gcc NUVIO_DTS_CXX=$SDK/bin/arm-webos-linux-gnueabi-g++
export NUVIO_BUILD_JOBS=10 NUVIO_P2P_MOTOR=none NUVIO_ARES_PACKAGE=$(which ares-package)
export ARM_BODY="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arm-body.sh"
export NUVIO_RELEASE_OUT=$HOME/opt/out
