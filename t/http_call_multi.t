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
