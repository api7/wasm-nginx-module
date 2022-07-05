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
BEGIN {
    $ENV{TEST_NGINX_USE_HUP} = 1;
}

use t::WASM 'no_plan';

run_tests();

__DATA__

=== TEST 1: sanity
--- http_config
    init_worker_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local p = wasm.load("plugin", "t/testdata/log/main.go.wasm")
        wasm.on_configure(p, "blah")
    }
--- grep_error_log eval
qr/\[\w+\] \d+#\d+: ouch, something is wrong/
--- grep_error_log_out eval
qr/\[emerg\] \d+#\d+: ouch, something is wrong
\[error\] \d+#\d+: ouch, something is wrong
\[warn\] \d+#\d+: ouch, something is wrong
\[info\] \d+#\d+: ouch, something is wrong
/
--- config
    location /t {
        return 200;
    }
--- no_error_log
[alert]



=== TEST 2: after receiving SIGHUP
--- http_config
    init_worker_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        local p = wasm.load("plugin", "t/testdata/log/main.go.wasm")
        wasm.on_configure(p, "blah")
    }
--- grep_error_log eval
qr/\[\w+\] \d+#\d+: ouch, something is wrong/
--- grep_error_log_out eval
qr/\[emerg\] \d+#\d+: ouch, something is wrong
\[error\] \d+#\d+: ouch, something is wrong
\[warn\] \d+#\d+: ouch, something is wrong
\[info\] \d+#\d+: ouch, something is wrong
/
--- config
    location /t {
        return 200;
    }
--- no_error_log
[alert]
