use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: sanity
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_call/main.go.wasm"))
        local data = json.encode({
            {host = "127.0.0.1:1980"},
            {host = "127.0.0.1:1981"},
        })
        local ctx = assert(wasm.on_configure(plugin, data))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/multiple calls are not supported/
--- grep_error_log_out
multiple calls are not supported
--- error_log
httpcall failed:
