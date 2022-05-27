<!--
  ~ Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
  ~
  ~ Licensed under the Apache License, Version 2.0 (the "License");
  ~ you may not use this file except in compliance with the License.
  ~ You may obtain a copy of the License at
  ~
  ~ http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing, software
  ~ distributed under the License is distributed on an "AS IS" BASIS,
  ~ WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  ~ See the License for the specific language governing permissions and
  ~ limitations under the License.
  ~
-->
## Status

This library is under construction. See https://github.com/api7/wasm-nginx-module/issues/25 to know the progress.

## Description

A Nginx module which tries to implement [proxy wasm ABI](https://github.com/proxy-wasm/spec) in Nginx.
The [Wasm integration of Apache APISIX](https://github.com/apache/apisix/blob/master/docs/en/latest/wasm.md) is powered by this module.

## Install dependencies

* Download the wasmtime C API package and rename it to `wasmtime-c-api/`, with the `./install-wasmtime.sh`.
Remember to add the `wasmtime-c-api/lib` to the library search path when you build Nginx, for instance,

```
export wasmtime_prefix=/path/to/wasm-nginx-module/wasmtime-c-api
./configure ... \
    --with-ld-opt="-Wl,-rpath,${wasmtime_prefix}/lib" \
```

* Download WasmEdge with the `./install-wasmedge.sh`.
Remember to add the `$HOME/.wasmedge/lib` to the library search path when you build Nginx, for instance,

```
./configure ... \
    --with-ld-opt="-Wl,-rpath,${HOME}/.wasmedge/lib" \
```

## Directives

### wasm_vm

**syntax:** *wasm_vm wasmtime|wasmedge*

**default:** -

**context:** *http*

Select the WASM VM. Currently, only wasmtime and WasmEdge are supported.
If the directive is not set, the WASM VM won't be enabled.

## Methods

**Remember to set the `wasm_vm` directive!**

### load

`syntax: plugin, err = proxy_wasm.load(name, path)`

Load a `.wasm` file from the filesystem and return a Wasm plugin.

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

### on_http_request_body

`syntax: ok, err = proxy_wasm.on_http_request_body(plugin_ctx, body, end_of_body)`

Run the HTTP request body filter in the plugin of the given plugin ctx.

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
-- get_body is a utility method to get the whole request body
local body = request.get_body()
-- if the body is not the whole request body, for example, it comes from
-- lua-resty-upload, remember to set end_of_body to false
assert(wasm.on_http_request_body(ctx, body, true))
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

### on_http_response_body

`syntax: ok, err = proxy_wasm.on_http_response_body(plugin_ctx)`

Run the HTTP response body filter in the plugin of the given plugin ctx.
This method need to be called in `body_filter_by_lua` phase and may be run
multiple times.

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
assert(wasm.on_http_response_body(ctx))
```

## proxy-wasm ABI

Implemented proxy-wasm ABI can be found in [proxy_wasm_abi](./proxy_wasm_abi.md).
