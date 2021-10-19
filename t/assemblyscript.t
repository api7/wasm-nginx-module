use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: fault injection
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("FaultInjection",
            "t/testdata/assemblyscript/build/untouched.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'body'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 403
--- response_body chomp
body
