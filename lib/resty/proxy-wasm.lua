-- Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
-- http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
local ffi = require("ffi")
local base = require("resty.core.base")
local http = require("resty.http")
local ffi_cast = ffi.cast
local ffi_gc = ffi.gc
local ffi_str = ffi.string
local C = ffi.C
local get_request = base.get_request
local get_string_buf = base.get_string_buf
local NGX_OK = base.FFI_OK


base.allows_subsystem("http")


ffi.cdef[[
typedef unsigned char u_char;
typedef long ngx_int_t;
typedef unsigned long ngx_uint_t;

typedef struct {
    size_t      len;
    u_char     *data;
} ngx_str_t;

typedef struct {
    int         len;
    u_char     *data;
} ngx_http_lua_ffi_str_t;

typedef struct {
    ngx_http_lua_ffi_str_t  key;
    ngx_http_lua_ffi_str_t  value;
} proxy_wasm_table_elt_t;

void *ngx_http_wasm_load_plugin(const char *name, size_t name_len, const char *code, size_t size);
void ngx_http_wasm_unload_plugin(void *plugin);
void *ngx_http_wasm_on_configure(void *plugin, const char *conf, size_t size);
void ngx_http_wasm_delete_plugin_ctx(void *hwp_ctx);

ngx_int_t ngx_http_wasm_on_http(void *hwp_ctx, void *r, int type,
                                const u_char *body, size_t size, int end_of_body);
ngx_str_t *ngx_http_wasm_fetch_local_body(void *r);

ngx_int_t ngx_http_wasm_call_max_headers_count(void *r);
void ngx_http_wasm_call_get(void *r, ngx_str_t *method, ngx_str_t *scheme,
                            ngx_str_t *host, ngx_str_t *path,
                            proxy_wasm_table_elt_t *headers, ngx_str_t *body,
                            int32_t *timeout);
ngx_int_t ngx_http_wasm_on_http_call_resp(void *hwp_ctx, ngx_http_request_t *r,
                                          proxy_wasm_table_elt_t *headers, ngx_uint_t n_header,
                                          ngx_str_t *body);
]]
local ngx_table_type = ffi.typeof("proxy_wasm_table_elt_t*")
local ngx_table_size = ffi.sizeof("proxy_wasm_table_elt_t")


local _M = {}
local HTTP_REQUEST_HEADERS = 1
local HTTP_REQUEST_BODY = 2
local HTTP_RESPONSE_HEADERS = 4
local HTTP_RESPONSE_BODY = 8

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

        local body
        if body_p[0].len > 0 then
            body = ffi_str(body_p[0].data, body_p[0].len)
        end

        local timeout = tonumber(timeout_p[0])

        ngx.log(ngx.NOTICE, "send http request to ", uri, ", method: ", method,
                ", timeout in ms: ", timeout)

        local httpc = http.new()
        httpc:set_timeout(timeout)
        local res, err = httpc:request_uri(uri, {
            method = method,
            headers = headers,
            body = body,
        })
        if not res then
            ngx.log(ngx.ERR, "http call failed: ", err, ", uri: ", uri)
        end

        return res
    end
end


local function handle_http_callback(plugin_ctx, r, res)
    local headers_buf, body
    local n = 0
    -- always trigger http call callback, even the http call failed
    if res then
        local hdrs = res.headers
        for name, value in pairs(hdrs) do
            if type(value) == "table" then
                n = n + #value
            else
                n = n + 1
            end
        end

        local i = 0
        local raw_buf = get_string_buf(n * ngx_table_size)
        headers_buf = ffi_cast(ngx_table_type, raw_buf)
        for name, value in pairs(hdrs) do
            name = string.lower(name)
            if type(value) == "table" then
                for _, v in ipairs(value) do
                    headers_buf[i].key.data = name
                    headers_buf[i].key.len = #name
                    headers_buf[i].value.data = v
                    headers_buf[i].value.len = #v
                    i = i + 1
                end
            else
                headers_buf[i].key.data = name
                headers_buf[i].key.len = #name
                headers_buf[i].value.data = value
                headers_buf[i].value.len = #value
                i = i + 1
            end
        end

        headers_buf[n].key.data = ":status"
        headers_buf[n].key.len = 7
        local status = tostring(res.status)
        headers_buf[n].value.data = status
        headers_buf[n].value.len = #status
        n = n + 1

        if res.body then
            body = ffi.new("ngx_str_t")
            body.data = res.body
            body.len = #res.body
        end
    end

    return C.ngx_http_wasm_on_http_call_resp(plugin_ctx, r, headers_buf, n, body)
end


local function on_http_request(plugin_ctx, ty, body, end_of_body)
    if type(plugin_ctx) ~= "cdata" then
        return nil, "bad plugin ctx"
    end

    local r = get_request()
    if not r then
        return nil, "bad request"
    end

    local err
    if ty == HTTP_REQUEST_HEADERS then
        err = "failed to run proxy_on_http_request_headers"
    else
        err = "failed to run proxy_on_http_request_body"
    end

    local body_size
    if not body then
        body_size = 0
    else
        body_size = #body
    end

    local rc = C.ngx_http_wasm_on_http(plugin_ctx, r, ty, body, body_size, end_of_body)
    if rc < 0 then
        return nil, err
    end

    while true do
        if rc >= 100 then
            local p = C.ngx_http_wasm_fetch_local_body(r)
            if p ~= nil then
                local body = ffi_str(p.data, p.len)
                ngx.status = rc
                ngx.print(body)
            end

            return ngx.exit(rc)
        end

        if rc == RC_NEED_HTTP_CALL then
            local res = send_http_call(r)
            rc = handle_http_callback(plugin_ctx, r, res)
            if rc < 0 then
                return nil, "failed to run proxy_on_http_call_response"
            end

            -- continue to handle other rc
        end

        if rc == NGX_OK then
            break
        end
    end

    return true
end


function _M.on_http_request_headers(plugin_ctx)
    return on_http_request(plugin_ctx, HTTP_REQUEST_HEADERS, nil, 1)
end


function _M.on_http_request_body(plugin_ctx, body, end_of_body)
    return on_http_request(plugin_ctx, HTTP_REQUEST_BODY, body, end_of_body and 1 or 0)
end


function _M.on_http_response_headers(plugin_ctx)
    if type(plugin_ctx) ~= "cdata" then
        return nil, "bad plugin ctx"
    end

    local r = get_request()
    if not r then
        return nil, "bad request"
    end

    local rc = C.ngx_http_wasm_on_http(plugin_ctx, r, HTTP_RESPONSE_HEADERS, nil, 0, 1)
    if rc < 0 then
        return nil, "failed to run proxy_on_http_response_headers"
    end

    return true
end


function _M.on_http_response_body(plugin_ctx)
    if type(plugin_ctx) ~= "cdata" then
        return nil, "bad plugin ctx"
    end

    local r = get_request()
    if not r then
        return nil, "bad request"
    end

    -- TODO: rewrite this by exporting FFI interfaces in OpenResty to save a copy
    local body = ngx.arg[1]
    local eof = ngx.arg[2]
    local rc = C.ngx_http_wasm_on_http(plugin_ctx, r, HTTP_RESPONSE_BODY, body, #body, eof)
    if rc < 0 then
        return nil, "failed to run proxy_on_http_response_body"
    end

    return true
end


return _M
