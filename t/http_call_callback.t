use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: get all headers
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "headers"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_log
get numHeaders 6
--- no_error_log
[error]
--- grep_error_log eval
qr/get header \S+: \S+/
--- grep_error_log_out eval
qr/get header :status: 200,
get header connection: keep-alive,
get header content-type: text\/plain,
get header date: \w+,
get header server: openresty\/[^,]+,
get header transfer-encoding: chunked,
/



=== TEST 2: get all headers, with repeated headers
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", path = "/repeated_headers", action = "headers"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_log
get numHeaders 8
--- no_error_log
[error]
--- grep_error_log eval
qr/get header \S+: \S+/
--- grep_error_log_out eval
qr/get header :status: 200,
get header connection: keep-alive,
get header content-type: text\/plain,
get header date: \w+,
get header foo: bar,
get header foo: baz,
get header server: openresty\/[^,]+,
get header transfer-encoding: chunked,
/



=== TEST 3: callback when the request failed
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1979", action = "headers"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_log
callback err: error status returned by host: not found



=== TEST 4: get body, no body
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "body"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_log
get bodySize 0
error status returned by host: not found



=== TEST 5: get body
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "body", path = "/?body=helloworld"}
        for _, e in ipairs({
            {0, 2},
            {1, 1},
            {1, 2},
            {0, 10},
            {1, 9},
            {8, 2},
            {9, 1},
            {1, 10},
        }) do
            conf.start = e[1]
            conf.size = e[2]
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
    }
}
--- grep_error_log eval
qr/get body \[\w+\]/
--- grep_error_log_out
get body [he]
get body [e]
get body [el]
get body [helloworld]
get body [elloworld]
get body [ld]
get body [d]
get body [elloworld]



=== TEST 6: get body, bad cases
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "body", path = "/?body=helloworld"}
        for _, e in ipairs({
            {-1, 2},
            {1, 0},
            {1, -1},
            {10, 2},
        }) do
            conf.start = e[1]
            conf.size = e[2]
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
    }
}
--- error_log
[error]
--- grep_error_log eval
qr/error status returned by host: bad argument/
--- grep_error_log_out eval
"error status returned by host: bad argument\n" x 4



=== TEST 7: call then send
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "then_send"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 503
--- grep_error_log eval
qr/run http callback callout id: \d+, plugin ctx id: \d+/
--- grep_error_log_out
run http callback callout id: 0, plugin ctx id: 1



=== TEST 8: call then call
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "then_call"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 403
--- grep_error_log eval
qr/run http callback callout id: \d+, plugin ctx id: \d+/
--- grep_error_log_out
run http callback callout id: 0, plugin ctx id: 1
run http callback callout id: 1, plugin ctx id: 1



=== TEST 9: call x 3
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local conf = {host = "127.0.0.1:1980", action = "then_call_again"}
        local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 401
--- grep_error_log eval
qr/run http callback callout id: \d+, plugin ctx id: \d+/
--- grep_error_log_out
run http callback callout id: 0, plugin ctx id: 1
run http callback callout id: 1, plugin ctx id: 1
run http callback callout id: 2, plugin ctx id: 1
