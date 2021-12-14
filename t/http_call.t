use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: call in response
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'call_in_resp'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- error_log
http call is only supported during processing HTTP request



=== TEST 2: invalid host
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local input = {
            {host = ""},
            {host = "127.0.0.1:111981"},
            {host = "a::1]"},
        }
        for i, conf in ipairs(input) do
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
    }
}
--- grep_error_log eval
qr/http call gets invalid host:/
--- grep_error_log_out
http call gets invalid host:
http call gets invalid host:
http call gets invalid host:
--- error_log
httpcall failed:



=== TEST 3: sanity
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/called for contextID = \d+/
--- grep_error_log_out
called for contextID = 2



=== TEST 4: connect refused
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1981"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        local ok, err = wasm.on_http_request_headers(ctx)
        if not ok then
            ngx.say(err)
        end
    }
}
--- grep_error_log eval
qr/called for contextID = \d+/
--- grep_error_log_out
called for contextID = 2
--- error_log
connection refused



=== TEST 5: panic
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "panic"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        local ok, err = wasm.on_http_request_headers(ctx)
        if not ok then
            ngx.say(err)
        end
    }
}
--- error_log
[error]
--- response_body
failed to run proxy_on_http_call_response



=== TEST 6: call and send (handle call, skip send)
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "call_and_send"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/called for contextID = \d+/
--- grep_error_log_out
called for contextID = 2



=== TEST 7: multiple http calls in different requests
--- config
location /call {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
location /t {
    content_by_lua_block {
        local http = require "resty.http"
        local uri = "http://127.0.0.1:" .. ngx.var.server_port .. "/call"
        local t = {}
        for i = 1, 18 do
            local th = assert(ngx.thread.spawn(function(i)
                local httpc = http.new()
                local res, err = httpc:request_uri(uri, {method = "GET"})
                if not res then
                    ngx.log(ngx.ERR, err)
                    return
                end
                if res.status ~= 200 then
                    ngx.log(ngx.ERR, "unexpected status: ", res.status)
                    return
                end
            end, i))
            table.insert(t, th)
        end
        for i, th in ipairs(t) do
            ngx.thread.wait(th)
        end
    }
}
--- timeout: 5
--- grep_error_log eval
qr/called for contextID = \d+/
--- grep_error_log_out eval
"called for contextID = 2\n" x 18



=== TEST 8: multiple http calls in different requests, the same plugin ctx
--- config
location /call {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        assert(wasm.on_http_request_headers(package.loaded.ctx))
    }
}
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980"}
        package.loaded.ctx = assert(wasm.on_configure(plugin, json.encode(conf)))

        local http = require "resty.http"
        local uri = "http://127.0.0.1:" .. ngx.var.server_port .. "/call"
        local t = {}
        for i = 1, 18 do
            local th = assert(ngx.thread.spawn(function(i)
                local httpc = http.new()
                local res, err = httpc:request_uri(uri, {method = "GET"})
                if not res then
                    ngx.log(ngx.ERR, err)
                    return
                end
                if res.status ~= 200 then
                    ngx.log(ngx.ERR, "unexpected status: ", res.status)
                    return
                end
            end, i))
            table.insert(t, th)
        end
        for i, th in ipairs(t) do
            ngx.thread.wait(th)
        end
    }
}
--- timeout: 5
--- grep_error_log eval
qr/called for contextID +/
--- grep_error_log_out eval
"called for contextID \n" x 18
