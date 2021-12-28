#!/bin/bash

WASM_PREFIX=$(pwd)
OPENRESTY_PREFIX=$(pwd)/../openresty-1.19.9.1
OPENRESTY_BINARY_PREFIX=${OPENRESTY_PREFIX}/output/bin

# build openresty
cd ../openresty-1.19.9.1

#if [ "$1" && "$1" == "reconfig" ] then
#    ./configure --prefix=$(pwd)/output -j12 \
#        --add-module=${WASM_PREFIX} \
#        --with-ld-opt="-Wl,-rpath,${WASM_PREFIX}/wasmtime-c-api/lib" \
#        --with-cc-opt="-O0"
#    exit 0
#fi

sudo make
sudo make install

sudo ${OPENRESTY_BINARY_PREFIX}/openresty -s stop || echo 1
#sudo ${OPENRESTY_BINARY_PREFIX}/openresty