local ffi = require("ffi")
local base = require("resty.core.base")
local ffi_gc = ffi.gc
local C = ffi.C
local get_request = base.get_request


base.allows_subsystem("http")


ffi.cdef[[
typedef long ngx_int_t;
void *ngx_http_wasm_load_plugin(const char *code, size_t size);
void ngx_http_wasm_unload_plugin(void *plugin);
void *ngx_http_wasm_on_configure(void *plugin, const char *conf, size_t size);
void ngx_http_wasm_delete_plugin_ctx(void *hwp_ctx);
ngx_int_t ngx_http_wasm_on_http(void *hwp_ctx, void *r, int type);
]]


local _M = {}
local HTTP_REQUEST_HEADERS = 1


function _M.load(path)
    local f, err = io.open(path)
    if not f then
        return nil, err
    end

    local data, err = f:read("*a")
    f:close()
    if not data then
        return nil, err
    end

    local plugin = C.ngx_http_wasm_load_plugin(data, #data)
    if plugin == nil then
        return nil, "failed to load wasm plugin"
    end

    ffi_gc(plugin, C.ngx_http_wasm_unload_plugin)

    return plugin
end


function _M.on_configure(plugin, conf)
    if type(plugin) ~= "cdata" then
        return nil, "bad plugin"
    end

    conf = conf or ""
    local plugin_ctx = C.ngx_http_wasm_on_configure(plugin, conf, #conf)
    if plugin_ctx == nil then
        return nil, "failed to run proxy_on_configure"
    end

    ffi_gc(plugin_ctx, C.ngx_http_wasm_delete_plugin_ctx)

    return plugin_ctx
end


function _M.on_http_request_headers(plugin_ctx)
    if type(plugin_ctx) ~= "cdata" then
        return nil, "bad plugin ctx"
    end

    local r = get_request()
    if not r then
        return nil, "bad request"
    end

    local rc = C.ngx_http_wasm_on_http(plugin_ctx, r, HTTP_REQUEST_HEADERS)
    if rc < 0 then
        return nil, "failed to run proxy_on_http_request_headers"
    end

    return true
end


return _M
