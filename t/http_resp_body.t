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

=== TEST 1: sanity
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_body/main.go.wasm"))
        ngx.ctx.ctx = assert(wasm.on_configure(plugin, '{}'))

        ngx.print("1234")
        ngx.print("12345")
    }
    body_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        ngx.log(ngx.WARN, ngx.arg[1], " ", ngx.arg[2])
        local ctx = ngx.ctx.ctx
        assert(wasm.on_http_response_body(ctx))
    }
}
--- grep_error_log eval
qr/body size \d+, end of stream \w+/
--- grep_error_log_out
body size 4, end of stream false
body size 5, end of stream false
body size 0, end of stream true



=== TEST 2: flush
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_body/main.go.wasm"))
        ngx.ctx.ctx = assert(wasm.on_configure(plugin, '{}'))

        ngx.print("1234")
        ngx.flush()
        ngx.print("12345")
    }
    body_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local ctx = ngx.ctx.ctx
        assert(wasm.on_http_response_body(ctx))
    }
}
--- grep_error_log eval
qr/body size \d+, end of stream \w+/
--- grep_error_log_out
body size 4, end of stream false
body size 0, end of stream false
body size 5, end of stream false
body size 0, end of stream true



=== TEST 3: no body
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_body/main.go.wasm"))
        ngx.ctx.ctx = assert(wasm.on_configure(plugin, '{"action":"offset"}'))
    }
    body_filter_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local ctx = ngx.ctx.ctx
        assert(wasm.on_http_response_body(ctx))
    }
}
--- grep_error_log eval
qr/body size \d+, end of stream \w+/
--- grep_error_log_out
body size 0, end of stream true
--- error_log
get body err: error status returned by host: not found



=== TEST 4: get body
--- config
location /t {
    return 200 "helloworld";
    body_filter_by_lua_block {
        if not ngx.arg[2] then
            return
        end

        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_body/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"action":"offset"}'))
        local conf = {action = "offset"}
        for _, e in ipairs({
            {0, 2},
            {1, 1},
            {1, 2},
            {0, 10},
            {1, 9},
            {8, 2},
            {9, 1},
            {1, 10},
        }) do
            conf.start = e[1]
            conf.size = e[2]
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            ngx.arg[1] = "helloworld"
            assert(wasm.on_http_response_body(ctx))
        end
    }
}
--- grep_error_log eval
qr/get body \[\w+\]/
--- grep_error_log_out
get body [he]
get body [e]
get body [el]
get body [helloworld]
get body [elloworld]
get body [ld]
get body [d]
get body [elloworld]



=== TEST 5: get body, bad cases
--- config
location /t {
    return 200 "helloworld";
    body_filter_by_lua_block {
        if not ngx.arg[2] then
            return
        end

        local json = require("cjson")
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_body/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"action":"offset"}'))
        local conf = {action = "offset"}
        for _, e in ipairs({
            {-1, 2},
            {1, 0},
            {1, -1},
            {10, 2},
        }) do
            conf.start = e[1]
            conf.size = e[2]
            local ctx = assert(wasm.on_configure(plugin, json.encode(conf)))
            ngx.arg[1] = "helloworld"
            assert(wasm.on_http_response_body(ctx))
        end
    }
}
--- error_log
[error]
--- grep_error_log eval
qr/error status returned by host: bad argument/
--- grep_error_log_out eval
"error status returned by host: bad argument\n" x 4
