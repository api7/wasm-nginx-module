use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: response header add
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_add'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add: foo, bar



=== TEST 2: response header set
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_set'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add: bar



=== TEST 3: response header set empty
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_set_empty'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add:



=== TEST 4: response header get
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add: foo
--- grep_error_log eval
qr/get response header: \S+/
--- grep_error_log_out
get response header: foo,



=== TEST 5: response header get miss
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_miss'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header: \S+/
--- grep_error_log_out
get response header: [],



=== TEST 6: response header get first header
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_first'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add: foo, bar
--- grep_error_log eval
qr/get response header: \S+/
--- grep_error_log_out
get response header: foo,
