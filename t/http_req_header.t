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

=== TEST 1: get header
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: foo
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: foo,



=== TEST 2: get first header
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: bar
X-API: foo
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: bar,



=== TEST 3: get miss
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-APIA: bar
--- error_log
error status returned by host: not found



=== TEST 4: get header, ignoring case
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get_caseless'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: foo
--- grep_error_log eval
qr/get request header: \S+/
--- grep_error_log_out
get request header: foo,



=== TEST 5: get all header
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get_all'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: foo
--- grep_error_log eval
qr/get request header: [^,]+/
--- grep_error_log_out
get request header: host localhost
get request header: connection close
get request header: x-api foo
get request header: :scheme http
get request header: :path /t
get request header: :method GET



=== TEST 6: get all header, repeated headers
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_get_all'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- more_headers
X-API: foo
X-API: bar
--- grep_error_log eval
qr/get request header: [^,]+/
--- grep_error_log_out
get request header: host localhost
get request header: connection close
get request header: x-api foo
get request header: x-api bar
get request header: :scheme http
get request header: :path /t
get request header: :method GET



=== TEST 7: set header
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_set'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.var.http_foo)
    }
}
--- more_headers
FOO: foo
--- response_body
bar



=== TEST 8: set header, missing
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_set'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.var.http_foo)
    }
}
--- response_body
bar



=== TEST 9: add header
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_add'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.req.get_headers()["foo"])
    }
}
--- more_headers
FOO: foo
--- response_body
foobar



=== TEST 10: add header, missing
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_add'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.req.get_headers()["foo"])
    }
}
--- response_body
bar



=== TEST 11: remove header
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_del'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.req.get_headers()["foo"])
    }
}
--- more_headers
FOO: foo
--- response_body
nil



=== TEST 12: remove header, missing
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_del'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.req.get_headers()["foo"])
    }
}
--- response_body
nil



=== TEST 13: remove header, all of it
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_hdr_del'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.say(ngx.req.get_headers()["foo"])
    }
}
--- more_headers
FOO: foo
FOO: bar
--- response_body
nil



=== TEST 14: get path (not args)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_path_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/get request path: \S+/
--- grep_error_log_out
get request path: /t,



=== TEST 15: get path (args)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_path_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- request
GET /t?q=hello&a=world
--- grep_error_log eval
qr/get request path: \S+/
--- grep_error_log_out
get request path: /t?q=hello&a=world,



=== TEST 16: get method (GET)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_method_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/get request method: \S+/
--- grep_error_log_out
get request method: GET,



=== TEST 17: get method (POST)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_method_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- request
POST /t
--- grep_error_log eval
qr/get request method: \S+/
--- grep_error_log_out
get request method: POST,



=== TEST 18: get method (PUT)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_method_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- request
PUT /t
--- grep_error_log eval
qr/get request method: \S+/
--- grep_error_log_out
get request method: PUT,



=== TEST 19: get method (DELETE)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_method_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- request
DELETE /t
--- grep_error_log eval
qr/get request method: \S+/
--- grep_error_log_out
get request method: DELETE,



=== TEST 20: get schema(http2)
--- http2
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_scheme_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/get request scheme: \S+/
--- grep_error_log_out
get request scheme: http,



=== TEST 21: get header, abnormal
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'req_hdr_get_abnormal'}
        for _, e in ipairs({
            ""
        }) do
            conf.header = e
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
    }
}
--- more_headers
X-API: foo
--- error_log
error status returned by host: not found



=== TEST 22: set header, abnormal
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'req_hdr_set_abnormal'}
        for _, e in ipairs({
            {"", ""}
        }) do
            conf.header = e[1]
            conf.value = e[2]
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
    }
}



=== TEST 23: get schema(http)
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_scheme_get'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/get request scheme: \S+/
--- grep_error_log_out
get request scheme: http,



=== TEST 24: delete pseduo headers
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'req_pseduo_del'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_log
[error]
--- grep_error_log eval
qr/can't remove pseudo header/
--- grep_error_log_out
can't remove pseudo header
can't remove pseudo header



=== TEST 25: set path, abnormal
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'req_path_set'}
        for _, e in ipairs({
            "a\r\na",
        }) do
            conf.value = e
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
        ngx.say(ngx.var.uri)
    }
}
--- error_log
[error]
--- grep_error_log eval
qr/failed to set request header :path: invalid char/
--- grep_error_log_out
failed to set request header :path: invalid char
--- response_body
/t



=== TEST 26: set path
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'req_path_set'}
        for _, e in ipairs({
            "a",
            "ac",
            "a?",
            "a?a",
            "a?a=c",
        }) do
            conf.value = e
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
            ngx.say(ngx.var.uri)
        end
    }
}
--- response_body
a
ac
a?
a?a
a?a=c



=== TEST 27: set method, abnormal
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'req_method_set'}
        for _, e in ipairs({
            "a\nc",
            "a c",
        }) do
            conf.value = e
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
        end
        ngx.say(ngx.var.request_method)
    }
}
--- error_log
[error]
--- grep_error_log eval
qr/failed to set request header :method: invalid char/
--- grep_error_log_out
failed to set request header :method: invalid char
failed to set request header :method: invalid char
--- response_body
GET



=== TEST 28: set method
--- config
location /t {
    content_by_lua_block {
        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_header/main.go.wasm"))
        local conf = {action = 'req_method_set'}
        for _, e in ipairs({
            "GET",
            "POST",
            "PURGE"
        }) do
            conf.value = e
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            assert(wasm.on_http_request_headers(ctx))
            ngx.say(ngx.var.request_method)
        end
    }
}
--- response_body
GET
POST
PURGE
