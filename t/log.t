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
--- log_level: debug
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local p = wasm.load("plugin", "t/testdata/log/main.go.wasm")
        wasm.on_configure(p, "blah")
    }
}
--- grep_error_log eval
qr/\[\w+\] \d+#\d+: ouch, something is wrong/
--- grep_error_log_out eval
qr/\[emerg\] \d+#\d+: ouch, something is wrong
\[error\] \d+#\d+: ouch, something is wrong
\[warn\] \d+#\d+: ouch, something is wrong
\[info\] \d+#\d+: ouch, something is wrong
\[debug\] \d+#\d+: ouch, something is wrong
\[debug\] \d+#\d+: ouch, something is wrong
/
--- no_error_log
[alert]



=== TEST 2: log in http request
--- config
location /t {
    content_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local plugin = assert(wasm.load("plugin", "t/testdata/log/main.go.wasm"))
        local ctx = assert(wasm.on_configure(plugin, '{"body":512}'))
        assert(wasm.on_http_request_headers(ctx))
    }
}
--- grep_error_log eval
qr/(create|run) http ctx \d+, client/
--- grep_error_log_out
create http ctx 2, client
run http ctx 2, client
--- no_error_log
[alert]
