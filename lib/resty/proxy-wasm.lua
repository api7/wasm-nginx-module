local ffi = require("ffi")
local base = require("resty.core.base")
local http = require("resty.http")
local ffi_cast = ffi.cast
local ffi_gc = ffi.gc
local ffi_str = ffi.string
local C = ffi.C
local get_request = base.get_request
local get_string_buf = base.get_string_buf


base.allows_subsystem("http")


ffi.cdef[[
typedef unsigned char u_char;
typedef long ngx_int_t;

typedef struct {
    size_t      len;
    u_char     *data;
} ngx_str_t;

typedef struct {
    ngx_str_t   key;
    ngx_str_t   value;
} proxy_wasm_table_elt_t;

void *ngx_http_wasm_load_plugin(const char *name, size_t name_len, const char *code, size_t size);
void ngx_http_wasm_unload_plugin(void *plugin);
void *ngx_http_wasm_on_configure(void *plugin, const char *conf, size_t size);
void ngx_http_wasm_delete_plugin_ctx(void *hwp_ctx);

ngx_int_t ngx_http_wasm_on_http(void *hwp_ctx, void *r, int type);
ngx_str_t *ngx_http_wasm_fetch_local_body(void *r);

ngx_int_t ngx_http_wasm_call_max_headers_count(void *r);
void ngx_http_wasm_call_get(void *r, ngx_str_t *method, ngx_str_t *scheme,
                            ngx_str_t *host, ngx_str_t *path,
                            proxy_wasm_table_elt_t *headers, ngx_str_t *body,
                            int32_t *timeout);
ngx_int_t ngx_http_wasm_on_http_call_resp(void *hwp_ctx, ngx_http_request_t *r);
]]
local ngx_table_type = ffi.typeof("proxy_wasm_table_elt_t*")
local ngx_table_size = ffi.sizeof("proxy_wasm_table_elt_t")


local _M = {}
local HTTP_REQUEST_HEADERS = 1
--local HTTP_REQUEST_BODY = 2
local HTTP_RESPONSE_HEADERS = 4
--local HTTP_RESPONSE_BODY = 8

local RC_NEED_HTTP_CALL = 1


function _M.load(name, path)
    local f, err = io.open(path)
    if not f then
        return nil, err
    end

    local data, err = f:read("*a")
    f:close()
    if not data then
        return nil, err
    end

    local plugin = C.ngx_http_wasm_load_plugin(name, #name, data, #data)
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

    if type(conf) ~= "string" or conf == "" then
        return nil, "bad conf"
    end

    local plugin_ctx = C.ngx_http_wasm_on_configure(plugin, conf, #conf)
    if plugin_ctx == nil then
        return nil, "failed to run proxy_on_configure"
    end

    ffi_gc(plugin_ctx, C.ngx_http_wasm_delete_plugin_ctx)

    return plugin_ctx
end


local send_http_call
do
    local method_p = ffi.new("ngx_str_t[1]")
    local scheme_p = ffi.new("ngx_str_t[1]")
    local host_p = ffi.new("ngx_str_t[1]")
    local path_p = ffi.new("ngx_str_t[1]")
    local body_p = ffi.new("ngx_str_t[1]")
    local timeout_p = ffi.new("int32_t[1]")

    function send_http_call(r)
        local n = C.ngx_http_wasm_call_max_headers_count(r)
        local raw_buf = get_string_buf((n + 1) * ngx_table_size)
        local headers_buf = ffi_cast(ngx_table_type, raw_buf)

        C.ngx_http_wasm_call_get(r, method_p, scheme_p, host_p,
                                 path_p, headers_buf, body_p, timeout_p)
        local uri = ffi_str(host_p[0].data, host_p[0].len)

        local scheme = "http://"
        if scheme_p[0].len > 0 then
            scheme = ffi_str(scheme_p[0].data, scheme_p[0].len) .. "://"
        end

        uri = scheme .. uri
        if path_p[0].len > 0 then
            local path = ffi_str(path_p[0].data, path_p[0].len)
            if path:byte(1) ~= string.byte("/") then
                uri = uri .. "/" .. path
            else
                uri = uri .. path
            end
        end

        local method = "GET"
        if method_p[0].len > 0 then
            method = ffi_str(method_p[0].data, method_p[0].len)
        end

        local headers
        if headers_buf[0].key.len ~= 0 then
            headers = {}

            local max = tonumber(n - 1)
            for i = 0, max do
                local hdr = headers_buf[i]
                if hdr.key.len == 0 then
                    break
                end

                local k = ffi_str(hdr.key.data, hdr.key.len)
                local v = ffi_str(hdr.value.data, hdr.value.len)
                if headers[k] then
                    if type(headers[k]) ~= "table" then
                        headers[k] = {headers[k], v}
                    else
                        table.insert(headers[k], v)
                    end
                else
                    headers[k] = v
                end
            end
        end

        local timeout = tonumber(timeout_p[0])

        ngx.log(ngx.NOTICE, "send http request to ", uri, ", method: ", method,
                ", timeout in ms: ", timeout)

        local httpc = http.new()
        httpc:set_timeout(timeout)
        local res, err = httpc:request_uri(uri, {
            method = method,
            headers = headers,
        })
        if not res then
            ngx.log(ngx.ERR, "http call failed: ", err, ", uri: ", uri)
        end

        return res
    end
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

    if rc >= 100 then
        local p = C.ngx_http_wasm_fetch_local_body(r)
        if p ~= nil then
            local body = ffi_str(p.data, p.len)
            ngx.status = rc
            ngx.print(body)
        end

        ngx.exit(rc)
    end

    if rc == RC_NEED_HTTP_CALL then
        local res = send_http_call(r)
        if not res then
            -- always trigger http call callback, even the http call failed
        end

        -- TODO: pass call response
        local rc = C.ngx_http_wasm_on_http_call_resp(plugin_ctx, r)
        if rc < 0 then
            return nil, "failed to run proxy_on_http_call_response"
        end
        -- TODO: handle send local & dispatch http call
    end

    return true
end


function _M.on_http_response_headers(plugin_ctx)
    if type(plugin_ctx) ~= "cdata" then
        return nil, "bad plugin ctx"
    end

    local r = get_request()
    if not r then
        return nil, "bad request"
    end

    local rc = C.ngx_http_wasm_on_http(plugin_ctx, r, HTTP_RESPONSE_HEADERS)
    if rc < 0 then
        return nil, "failed to run proxy_on_http_response_headers"
    end

    return true
end


return _M
