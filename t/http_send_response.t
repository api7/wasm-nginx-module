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
        local plugin = assert(wasm.load("plugin", "t/testdata/http_send_response/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '403_body'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 403
--- response_body chomp
should not pass
--- response_headers
powered-by: proxy-wasm-go-sdk!!



=== TEST 2: without body
--- config
location /t {
    access_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_send_response/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '502'))
        assert(wasm.on_http_request_headers(ctx))
    }
    content_by_lua_block {
        ngx.log(ngx.ERR, "should not reach")
    }
}
--- error_code: 502
--- response_body_like eval
qr/<title>502 Bad Gateway<\/title>/



=== TEST 3: multi headers
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/http_send_response/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'multi_hdrs'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 401
--- response_headers
hi: spacewander
hello: spacewander, world
