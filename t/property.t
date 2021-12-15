use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: get_property (host)
--- config
location /t {
    content_by_lua_block {
        local test_cases = {
            "plugin_root_id", "scheme", "host", "uri", "arg_test", "request_uri",
        }

        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/property/main.go.wasm")
        for _, case in ipairs(test_cases) do
            local plugin_ctx, err = wasm.on_configure(plugin, case)
            assert(wasm.on_http_request_headers(plugin_ctx))
        end
    }
}
--- request
GET /t?test=yeah
--- error_log
get property: plugin_root_id = plugin
get property: scheme = http
get property: host = localhost
get property: uri = /t
get property: arg_test = yeah
get property: request_uri = /t?test=yeah



=== TEST 2: get_property (missing)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/property/main.go.wasm")
        local plugin_ctx, err = wasm.on_configure(plugin, "none")
        assert(wasm.on_http_request_headers(plugin_ctx))
    }
}
--- request
GET /t?test=yeah
--- error_log
error get property: error status returned by host: not found
