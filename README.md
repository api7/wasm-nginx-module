## Status

This library is under construction.

## Description

A Nginx module which tries to implement [proxy wasm ABI](https://github.com/proxy-wasm/spec) in Nginx.

## Install dependencies

* Download the wasmtime C API package, see https://docs.wasmtime.dev/c-api/.
Then rename it to `wasmtime-c-api/`. Remember to add the `wasmtime-c-api/lib` to
the library search path when you build Nginx, for instance,

```
export wasm_prefix=/path/to/wasm-nginx-module/wasmtime-c-api
./configure ... \
    --with-ld-opt="-Wl,-rpath,${wasm_prefix}/lib" \
```

## Methods

### load

`syntax: plugin, err = proxy_wasm.load(path)`

Load a `.wasm` file from the filesystem and return a WASM plugin.

```lua
local plugin, err = proxy_wasm.load("t/testdata/plugin_lifecycle/main.go.wasm")
```

### on_configure

`syntax: plugin_ctx, err = proxy_wasm.on_configure(plugin, conf)`

Create a plugin ctx with the given plugin and conf.

```lua
local plugin, err = proxy_wasm.load("t/testdata/plugin_lifecycle/main.go.wasm")
if not plugin then
    ngx.log(ngx.ERR, "failed to load wasm ", err)
    return
end
local ctx, err = wasm.on_configure(plugin, '{"body":512}')
if not ctx then
    ngx.log(ngx.ERR, "failed to create plugin ctx ", err)
    return
end
```

## proxy-wasm ABI

Implemented proxy-wasm ABI can be found in [proxy_wasm_abi](./proxy_wasm_abi.md).
