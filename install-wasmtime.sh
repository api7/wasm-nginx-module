#!/usr/bin/env bash

wget https://github.com/bytecodealliance/wasmtime/releases/download/v0.30.0/wasmtime-v0.30.0-x86_64-linux-c-api.tar.xz
tar -xvf ./wasmtime-v0.30.0-x86_64-linux-c-api.tar.xz
mv wasmtime-v0.30.0-x86_64-linux-c-api wasmtime-c-api
