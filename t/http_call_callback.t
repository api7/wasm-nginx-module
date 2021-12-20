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
