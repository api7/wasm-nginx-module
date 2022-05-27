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

=== TEST 1: fault injection
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("FaultInjection",
            "t/testdata/assemblyscript/build/untouched.wasm"))
        local ctx = assert(wasm.on_configure(plugin, 'body'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- error_code: 403
--- response_body chomp
body
