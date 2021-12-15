## Status

This library is under construction. See https://github.com/api7/wasm-nginx-module/issues/25 to know the progress.

## Description

A Nginx module which tries to implement [proxy wasm ABI](https://github.com/proxy-wasm/spec) in Nginx.

## Install dependencies

* Download the wasmtime C API package and rename it to `wasmtime-c-api/`, with the `./install-wasmtime.sh`.
Remember to add the `wasmtime-c-api/lib` to the library search path when you build Nginx, for instance,

```
export wasm_prefix=/path/to/wasm-nginx-module/wasmtime-c-api
./configure ... \
    --with-ld-opt="-Wl,-rpath,${wasm_prefix}/lib" \
```

## Directives

### wasm_vm

**syntax:** *wasm_vm wasmtime*

**default:** -

**context:** *http*

Select the WASM VM. Currently, only wasmtime is supported. If the directive is
not set, the WASM VM won't be enabled.

## Methods

**Remember to set the `wasm_vm` directive!**

### load

`syntax: plugin, err = proxy_wasm.load(name, path)`

Load a `.wasm` file from the filesystem and return a WASM plugin.

```lua
local plugin, err = proxy_wasm.load("plugin","t/testdata/plugin_lifecycle/main.go.wasm")
```

### on_configure

`syntax: plugin_ctx, err = proxy_wasm.on_configure(plugin, conf)`

Create a plugin ctx with the given plugin and conf.

```lua
local plugin, err = proxy_wasm.load("plugin","t/testdata/plugin_lifecycle/main.go.wasm")
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

### on_http_request_headers

`syntax: ok, err = proxy_wasm.on_http_request_headers(plugin_ctx)`

Run the HTTP request headers filter in the plugin of the given plugin ctx.

```lua
local plugin, err = proxy_wasm.load("plugin","t/testdata/plugin_lifecycle/main.go.wasm")
if not plugin then
    ngx.log(ngx.ERR, "failed to load wasm ", err)
    return
end
local ctx, err = wasm.on_configure(plugin, '{"body":512}')
if not ctx then
    ngx.log(ngx.ERR, "failed to create plugin ctx ", err)
    return
end
assert(wasm.on_http_request_headers(ctx))
```

### on_http_response_headers

`syntax: ok, err = proxy_wasm.on_http_response_headers(plugin_ctx)`

Run the HTTP response headers filter in the plugin of the given plugin ctx.

```lua
local plugin, err = proxy_wasm.load("plugin","t/testdata/http_lifecycle/main.go.wasm")
if not plugin then
    ngx.log(ngx.ERR, "failed to load wasm ", err)
    return
end
local ctx, err = wasm.on_configure(plugin, '{"body":512}')
if not ctx then
    ngx.log(ngx.ERR, "failed to create plugin ctx ", err)
    return
end
assert(wasm.on_http_response_headers(ctx))
```

## proxy-wasm ABI

Implemented proxy-wasm ABI can be found in [proxy_wasm_abi](./proxy_wasm_abi.md).
