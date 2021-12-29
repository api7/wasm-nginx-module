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
        local plugin = wasm.load("plugin", "t/testdata/property/get.go.wasm")
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
        local plugin = wasm.load("plugin", "t/testdata/property/get.go.wasm")
        local plugin_ctx, err = wasm.on_configure(plugin, "none")
        assert(wasm.on_http_request_headers(plugin_ctx))
    }
}
--- request
GET /t?test=yeah
--- error_log
error get property: error status returned by host: not found



=== TEST 3: set_property (custom variable: success)
--- config
location /t {
    set $for_test origin_value;
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/property/set.go.wasm")
        local plugin_ctx, err = wasm.on_configure(plugin, 'for_test|new_value')
        ngx.say(ngx.var.for_test)
        ngx.say(#ngx.var.for_test)
        assert(wasm.on_http_request_headers(plugin_ctx))
        ngx.say(ngx.var.for_test)
        ngx.say(#ngx.var.for_test)
    }
}
--- response_body
origin_value
12
new_value
9
--- error_log
set property success: for_test = new_value



=== TEST 4: set_property (host: unchangeable)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/property/set.go.wasm")
        local plugin_ctx, err = wasm.on_configure(plugin, 'host|test.com')
        assert(wasm.on_http_request_headers(plugin_ctx))
    }
}
--- error_log
variable "host" not changeable



=== TEST 5: set_property (non-existent variable)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = wasm.load("plugin", "t/testdata/property/set.go.wasm")
        local plugin_ctx, err = wasm.on_configure(plugin, 'other|new_value')
        assert(wasm.on_http_request_headers(plugin_ctx))
    }
}
--- error_log
variable "other" not found for writing
error set property: error status returned by host: not found
