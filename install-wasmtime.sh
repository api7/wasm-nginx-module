#!/usr/bin/env bash
# Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
set -euo pipefail -x
arch=$(uname -m | tr '[:upper:]' '[:lower:]')
os=$(uname -s | tr '[:upper:]' '[:lower:]')
if [ "$os" = "darwin" ]; then
    os="macos"
    if [ "$arch" = "arm64" ]; then
        arch="aarch64"
    fi
else
    os="linux"
fi
ARCH=$arch
VER=v0.38.1
wget https://github.com/bytecodealliance/wasmtime/releases/download/${VER}/wasmtime-${VER}-${ARCH}-${os}-c-api.tar.xz
tar -xvf ./wasmtime-${VER}-${ARCH}-${os}-c-api.tar.xz > /dev/null
if [ -d wasmtime-c-api ]; then
    rm -rf wasmtime-c-api
fi
mv wasmtime-${VER}-${ARCH}-${os}-c-api wasmtime-c-api
if { echo "int main(void) {}" | gcc -o /dev/null -v -x c - &> /dev/stdout| grep collect | tr -s " " "\012" | grep musl; } \
    || ( [[ -f /etc/redhat-release ]] && [[ "$arch" = "aarch64" ]] ); then
    # build from source code if the libc is musl
    git clone https://github.com/bytecodealliance/wasmtime -b ${VER} --depth 1 \
        && cd wasmtime \
        && git submodule update --init \
        && RUSTFLAGS="-C target-feature=-crt-static" \
           cargo build --release --manifest-path crates/c-api/Cargo.toml \
        && mv target/release/libwasmtime.* ../wasmtime-c-api/lib \
        && cd ..
fi
