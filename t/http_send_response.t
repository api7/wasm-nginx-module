use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: sanity
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_send_response/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '403_body'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 403
--- response_body chomp
should not pass



=== TEST 2: without body
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_send_response/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '502'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 502
--- response_body chomp
