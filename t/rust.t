use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: fault injection
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("fault_injection",
            "t/testdata/rust/target/wasm32-wasi/debug/fault_injection.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"http_status": 403, "body": "body"}'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 403
--- response_body chomp
body



=== TEST 2: fault injection (without body)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("fault_injection",
            "t/testdata/rust/target/wasm32-wasi/debug/fault_injection.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"http_status": 401}'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 401
--- response_body_like eval
qr/<title>401 Authorization Required<\/title>/



=== TEST 3: fault injection (0 percentage)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("fault_injection",
            "t/testdata/rust/target/wasm32-wasi/debug/fault_injection.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"http_status": 401, "percentage": 0}'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
