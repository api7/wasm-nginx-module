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
--- error_log
error status returned by host: not found



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



=== TEST 7: response header delete
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_del'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add:



=== TEST 8: response header delete all
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_del_all'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add:



=== TEST 9: response header delete miss
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_del_miss'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- response_headers
add: foo



=== TEST 10: response header get all
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_all'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header \S+ \S+/
--- grep_error_log_out
get response header content-type text/plain,
get response header content-length 0,
get response header connection close,



=== TEST 11: response header get all, custom headers
--- config
location /t {
    return 200 "body";
    header_filter_by_lua_block {
        ngx.header["X-WASM"] = "true"
        ngx.header["X-foo"] = "bar"

        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_all'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header \S+ \S+/
--- grep_error_log_out
get response header x-wasm true,
get response header x-foo bar,
get response header content-type text/plain,
get response header content-length 4,
get response header connection close,



=== TEST 12: response header get all, repeated header
--- config
location /t {
    return 200 "body";
    header_filter_by_lua_block {
        local resp = require("ngx.resp")
        ngx.header["Content-Type"] = "text/html"
        ngx.header["X-WASM"] = "true"
        resp.add_header("X-WASM", "false")

        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_all'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header \S+ \S+/
--- grep_error_log_out
get response header x-wasm true,
get response header x-wasm false,
get response header content-type text/html,
get response header content-length 4,
get response header connection close,



=== TEST 13: response header get all, keepalive
--- config
location /up {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_all'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
location /t {
    content_by_lua_block {
        local http = require("resty.http")
        local uri = "http://127.0.0.1:" .. ngx.var.server_port .. "/up"
        local httpc = http.new()
        assert(httpc:request_uri(uri))
    }
}
--- grep_error_log eval
qr/get response header \S+ \S+/
--- grep_error_log_out
get response header content-type text/plain,
get response header content-length 0,
get response header connection keep-alive,



=== TEST 14: get header, abnormal
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'resp_hdr_get_abnormal'}
        for _, e in ipairs({
            ""
        }) do
            conf.header = e
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_response_headers(ctx))
        end
    }
}
--- error_log
error status returned by host: not found



=== TEST 15: set header, abnormal
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'resp_hdr_set_abnormal'}
        for _, e in ipairs({
            {"", ""}
        }) do
            conf.header = e[1]
            conf.value = e[2]
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_response_headers(ctx))
        end
    }
}
