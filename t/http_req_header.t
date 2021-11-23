use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: get header
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: foo
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: foo,



=== TEST 2: get first header
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: bar
X-API: foo
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: bar,



=== TEST 3: get miss
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-APIA: bar
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: ,



=== TEST 4: get header, ignoring case
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get_caseless'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: foo
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: foo,
