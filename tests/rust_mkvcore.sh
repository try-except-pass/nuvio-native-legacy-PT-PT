#!/bin/bash
# Standalone Rust checks with the same toolchain as product builds (tools/lg-host/env.sh).
set -eu
cd "$(dirname "$0")/.."
export RUSTUP_HOME="${RUSTUP_HOME:-$HOME/opt/rust/rustup}"
export CARGO_HOME="${CARGO_HOME:-$HOME/opt/rust/cargo}"
export PATH="$CARGO_HOME/bin:$PATH"
cargo clippy --manifest-path rust/mkvcore/Cargo.toml --all-targets --locked -- -D warnings
cargo test --manifest-path rust/mkvcore/Cargo.toml --locked
