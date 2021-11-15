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
