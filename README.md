## Install dependencies

* Download the wasmtime C API package, see https://docs.wasmtime.dev/c-api/.
Then rename it to `wasmtime-c-api/`. Remember to add the `wasmtime-c-api/lib` to
the library search path when you build Nginx, for instance,

```
export wasm_prefix=/path/to/wasm-nginx-module/wasmtime-c-api
./configure ... \
    --with-ld-opt="-Wl,-rpath,${wasm_prefix}/lib" \
```
