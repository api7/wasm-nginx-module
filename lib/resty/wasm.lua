local ffi = require("ffi")
local ffi_gc = ffi.gc
local C = ffi.C
local NGX_ERROR = ngx.ERROR


ffi.cdef[[
typedef long ngx_int_t;
void *ngx_http_load_plugin(const char *code, size_t size);
void ngx_http_unload_plugin(void *plugin);
]]


local _M = {}


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

    local plugin = C.ngx_http_load_plugin(data, #data)
    if plugin == nil then
        return nil, "failed to load wasm plugin"
    end

    ffi_gc(plugin, C.ngx_http_unload_plugin)

    return plugin
end


return _M
