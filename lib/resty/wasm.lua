local ffi = require("ffi")
local ffi_gc = ffi.gc
local C = ffi.C
local NGX_ERROR = ngx.ERROR
local NGX_OK = ngx.OK


ffi.cdef[[
typedef long ngx_int_t;
void *ngx_http_load_plugin(const char *code, size_t size);
void ngx_http_unload_plugin(void *plugin);
ngx_int_t ngx_http_on_configure(void *plugin, const char *conf, size_t size);
]]


local _M = {}


function _M.load(path, conf)
    local f, err = io.open(path)
    if not f then
        return nil, err
    end

    local data, err = f:read("*a")
    f:close()
    if not data then
        return nil, err
    end

    local plugin = C.ngx_http_load_plugin(data, #data)
    if plugin == nil then
        return nil, "failed to load wasm plugin"
    end

    ffi_gc(plugin, C.ngx_http_unload_plugin)

    local rc = C.ngx_http_on_configure(plugin, conf, #conf)
    if rc < 0 then
        return nil, "failed to run proxy_on_configure, rc: " .. tonumber(rc)
    end

    return plugin
end


return _M
