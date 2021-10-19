#!/usr/bin/env bash
set -euo pipefail -x

VER=v0.30.0
wget https://github.com/bytecodealliance/wasmtime/releases/download/${VER}/wasmtime-${VER}-x86_64-linux-c-api.tar.xz
tar -xvf ./wasmtime-${VER}-x86_64-linux-c-api.tar.xz > /dev/null
mv wasmtime-${VER}-x86_64-linux-c-api wasmtime-c-api
if echo "int main(void) {}" | gcc -o /dev/null -v -x c - &> /dev/stdout| grep collect | tr -s " " "\012" | grep musl; then
    # build from source code if the libc is musl
    git clone https://github.com/bytecodealliance/wasmtime -b ${VER} --depth 1 \
        && cd wasmtime \
        && git submodule update --init \
        && RUSTFLAGS="-C target-feature=-crt-static" \
           cargo build --release --manifest-path crates/c-api/Cargo.toml \
        && mv target/release/libwasmtime.* ../wasmtime-c-api/lib \
        && cd ..
fi
