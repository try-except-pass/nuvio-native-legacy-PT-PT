# Runs arm.sh's container step on this host (no docker). Sourced after tools/ctr.sh.
ctr_image_inspect() { echo host; }
ctr_run() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --env-file) set -a; . "$2"; set +a; shift 2;;
      -e) export "$2"; shift 2;;
      -v) case "$2" in *:/tmp/libmkvcore.a*) export NUVIO_RUST_LIB="${2%%:*}";; esac; shift 2;;
      --rm) shift;; --platform) shift 2;;
      bash) shift; break;;
      *) shift;;
    esac
  done
  export CCACHE_DIR="$PWD/build/ccache-arm"; mkdir -p "$CCACHE_DIR"
  export TMPW=$(mktemp -d)
  bash "$ARM_BODY"
}
