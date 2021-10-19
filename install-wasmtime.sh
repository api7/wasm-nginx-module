#!/usr/bin/env bash
set -euo pipefail -x

VER=v0.30.0
wget https://github.com/bytecodealliance/wasmtime/releases/download/${VER}/wasmtime-${VER}-x86_64-linux-c-api.tar.xz
tar -xvf ./wasmtime-${VER}-x86_64-linux-c-api.tar.xz > /dev/null
if echo "int main(void) {}" | gcc -o /dev/null -v -x c - &> /dev/stdout| grep collect | tr -s " " "\012" | grep musl; then
    # build from source code if the libc is musl
    git clone git@github.com:bytecodealliance/wasmtime -b ${VER} \
        && cd wasmtime \
        && git submodule update --init \
        && RUSTFLAGS="-C target-feature=-crt-static" \
           cargo build --release --manifest-path crates/c-api/Cargo.toml
fi
mv wasmtime-${VER}-x86_64-linux-c-api wasmtime-c-api
