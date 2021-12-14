use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: load
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        assert(wasm.load("plugin", "t/testdata/property/main.go.wasm"))
    }
}



=== TEST2: get_property
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/plugin_lifecycle/main.go.wasm")
        local plugin_ctx, err = wasm.on_configure(plugin, "test")
        assert(wasm.on_http_request_headers(test_ctx))
    }
}
--- request
GET /t?test=yeah
--- error_log
get property arg_test: yeah
get property scheme: http
get property host: localhost
get property request_uri: /t?test=yeah
get property uri: /t