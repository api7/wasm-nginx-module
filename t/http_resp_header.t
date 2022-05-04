# Copyright 2022 Shenzhen ZhiLiu Technology Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
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
get response header :status 200,



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
get response header :status 200,



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
get response header :status 200,



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
get response header :status 200,



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



=== TEST 16: get response status (200)
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_status'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header: \S+ \d+/
--- grep_error_log_out
get response header: :status 200



=== TEST 17: get response status (404)
--- config
location /t {
    return 404;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_status'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header: \S+ \d+/
--- grep_error_log_out
get response header: :status 404
--- error_code: 404



=== TEST 18: get response status (502)
--- config
location /t {
    return 502;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_get_status'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- grep_error_log eval
qr/get response header: \S+ \d+/
--- grep_error_log_out
get response header: :status 502
--- error_code: 502



=== TEST 19: response set status
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_set_status'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- error_code: 501



=== TEST 20: response set bad status
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_set_status_bad_val'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- error_log
invalid value



=== TEST 21: response del status
--- config
location /t {
    return 200;
    header_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'resp_hdr_del_status'))
        assert(wasm.on_http_response_headers(ctx))
    }
}
--- error_log
can't remove pseudo header
